"""Prove the hybrid runner draws what the port draws.

`tools/native/` runs the original binary under emulation with the port standing
in for the hardware and, routine by routine, for the code. This asks whether
what it *draws* is what the port draws - which is the only question that
matters about a graphics layer, and the one a run that merely fails to crash
does not answer.

**The two runs cannot be compared flip by flip.** They present frames at very
different rates, because their timer pacing is not the same: the port reaches
the title screen by its fiftieth flip and the hybrid by its four hundred and
eightieth. A frame-numbered comparison of the two says 83% of pixels differ and
means nothing at all - it is comparing different moments. That mistake cost an
afternoon here, so this aligns by *content* instead: it asks whether, for each
frame the port presents, the hybrid presents that frame exactly.

**A sampled frame can only land on a phase that is a multiple of the step.**
The title screen animates. Dumping every twentieth hybrid frame reproduced ten
of the port's sixteen title-screen flips byte for byte and missed six - and the
six were exactly the phases no multiple of twenty falls on. That reads as a
rendering fault and is a sampling one. So the hybrid dumps *every* frame across
a window, which is why the screens below carry one; a window is also what keeps
308 KB a frame from becoming a gigabyte over a whole run.

Matching a run of consecutive flips rather than one is the point. A single
frame can agree by luck on a screen that is mostly one colour; sixteen
consecutive flips agreeing, in order, is the animation itself. The last screen
here takes that as far as it goes: every flip the port presents in the whole
intro, all sixty-six of them, each one byte for byte.

The reference is a `.scrn` from `devtim`, the same capture `check_briefing.py`
compares against, so both sides are the port's own `vga_compose` and palette.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# What to prove, and where in each run to look for it.
#
#   the port flips that make up the screen, and the hybrid frames to dump.
#
# The window is deliberately wider than the flips it has to cover. The port
# paces on a wall clock, so which animation phase its flip 60 lands on moves
# between runs; a window sized to one run's worth reported five flips as
# unmatched whose closest frame was the last one in it, which is what the edge
# of a window always looks like.
#
# The intro's two logo screens live in one 480-line buffer on two pages - the
# Sierra logo in the displayed rows and Jeff Tunnell Productions lower down -
# so one flip carries both and is still, which is why one flip proves it. The
# title screen animates and takes a run.
SCREENS = (
    ("the intro logos",  (4,),                 (300, 400), 1),
    ("the title screen", tuple(range(50, 66)), (500, 720), 12),
    ("the whole intro",  tuple(range(0, 66)),  (1, 720),   60),
)


def need(path, how):
    if not os.path.exists(path):
        raise SystemExit("no %s - %s" % (os.path.relpath(path, ROOT), how))
    return path


def have(outdir, flips):
    """The wanted flips already captured in this directory."""
    return [f for f in flips
            if os.path.exists(os.path.join(outdir, "flip%04d.scrn" % f))]


def reference(flips, outdir, seconds):
    """Those flips out of the port, as `check_briefing.py` would take them."""
    got = have(outdir, flips)
    # Every wanted flip, or capture again. Counting instead of checking held a
    # threshold that the screens' own minimums summed to 73 when only 66 flips
    # exist, so reuse could never fire and a kept directory bought nothing.
    if len(got) == len(flips):
        print("port: reusing %d captured flips" % len(got))
    else:
        devtim = need(os.path.join(ROOT, "reconstruct", "devtim"),
                      "run `make -C reconstruct devtim`")
        # Leave as soon as the last wanted flip is written, for the same
        # reason the hybrid side does: otherwise the run is killed from
        # outside and costs its whole budget however early the flip arrived.
        env = dict(os.environ, SDL_VIDEODRIVER="dummy",
                   TIM_FLIPWANT=",".join(str(f) for f in flips),
                   TIM_STOPFLIP="%d" % max(flips),
                   TIM_FLIPS="%s:%d" % (outdir, max(flips)))
        try:
            subprocess.run([devtim], cwd=ROOT, env=env, timeout=seconds,
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
        except subprocess.TimeoutExpired:
            pass                   # neither side exits: a DOS game does not

    # Whatever it reached. How far the port gets in a fixed budget is wall
    # clock, not guest time - it reached flip 65 inside two minutes one run
    # and had not in five the next - so a screen asks for a range of flips and
    # a minimum, and is proved against the ones that came back. Demanding one
    # exact flip makes the tool report the port's pacing as a difference.
    out = {}
    for f in flips:
        path = os.path.join(outdir, "flip%04d.scrn" % f)
        if not os.path.exists(path):
            continue
        d = open(path, "rb").read()
        w, h, _vis = struct.unpack_from("<HHH", d, 8)
        out[f] = (d[14:14 + 768], d[-(w * h):])
    return out


def hybrid(window, outdir, seconds):
    """Every frame across the window, out of the hybrid runner."""
    native = need(os.path.join(ROOT, "tools", "native", "native"),
                  "run `make -C tools/native`")
    want = window[1] - window[0] + 1
    if len(glob.glob(os.path.join(outdir, "*.raw"))) >= want:
        print("hybrid: reusing %d captured frames" % want)
    else:
        # Stop the moment the window is complete. A DOS game does not exit,
        # so without this the run is killed from outside and costs the whole
        # budget however early the last frame arrived - 360 seconds for work
        # that takes eleven.
        env = dict(os.environ,
                   TIM_STOP="%d" % (window[1] + 1),
                   TIM_FRAMES="%s:1:%d:%d" % (outdir, window[0], window[1]))
        try:
            subprocess.run([native], cwd=ROOT, env=env, timeout=seconds,
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
        except subprocess.TimeoutExpired:
            pass                   # a DOS game does not exit; that is the plan

    # Digests, not pixels. Holding a whole window of frames costs 308 KB each
    # - 222 MB for a run of 720 - and every one of them gets hashed again to
    # build the index, which is minutes of work on a busy machine to answer a
    # question about equality. A digest answers it exactly and the bytes are
    # only needed when something fails, which is what the paths are for.
    out = {}
    for path in sorted(glob.glob(os.path.join(outdir, "*.raw"))):
        with open(path, "rb") as f:
            out[int(os.path.basename(path)[1:6])] = (
                hashlib.sha256(f.read()).digest(), path)
    return out


def differences(a, b):
    return sum(1 for x, y in zip(a, b) if x != y)


def index_of(frames):
    """Frame digest -> the frame numbers that have it, in order.

    Built once. A linear scan per flip is 66 x 720 comparisons of 307 KB once
    the whole intro is what is being checked.
    """
    out = {}
    for n in sorted(frames):
        out.setdefault(frames[n][0], []).append(n)
    return out


# How many failing flips get the expensive "closest frame" treatment. Finding
# it re-reads the whole window - 222 MB for a run of 720 - and when a screen
# fails it usually fails wholesale, so diagnosing every flip turns a failed
# check into a quarter of an hour of re-reading to say the same thing sixty-six
# times. The first few carry the information; the rest are a count.
DIAGNOSE = 3


def closest(pal, idx, frames):
    """The frame nearest this flip, and by how much - only used on a failure.

    The bytes are re-read here rather than kept, because this runs at most
    DIAGNOSE times per screen and never at all on a passing run.
    """
    best, diff, pdiff = None, None, None
    for n in sorted(frames):
        with open(frames[n][1], "rb") as f:
            d = f.read()
        this = differences(idx, d[768:])
        if diff is None or this < diff:
            best, diff = n, this
            pdiff = differences(pal, d[:768])
    return best, diff, pdiff


def check(name, flips, frames, index, least, seconds):
    """Every flip reproduced exactly, and the run of them in order."""
    print("\n%s - %d port flip%s"
          % (name, len(flips), "" if len(flips) == 1 else "s"))
    if len(flips) < least:
        print("  the port produced %d flips in %ds, and this screen needs %d. "
              "Raise --port-seconds" % (len(flips), seconds, least))
        return 1
    matched, bad = [], 0

    for f in sorted(flips):
        pal, idx = flips[f]
        hits = index.get(hashlib.sha256(pal + idx).digest())
        if hits:
            matched.append((f, hits))
            continue

        bad += 1
        if bad > DIAGNOSE:
            print("  flip %d: no frame matches." % f)
            continue
        near, diff, pdiff = closest(pal, idx, frames)
        print("  flip %d: no frame matches. Closest is f%05d, %d of %d pixels "
              "and %d of 768 palette bytes" % (f, near, diff, len(idx), pdiff))

    if bad:
        return bad

    print("  **every flip is reproduced byte for byte** - %d flips, %d pixels "
          "and 768 palette bytes each"
          % (len(matched), len(flips[matched[0][0]][1])))

    # The frames must be choosable in increasing order - one per flip, each
    # later than the last. Frames that each match *some* flip could otherwise
    # be the animation played backwards.
    #
    # It is a *selection* that has to be monotone, not the whole set of hits:
    # a frame's content recurs across a long stretch - a screen held still, a
    # blank between scenes - so one flip legitimately matches frames from all
    # over, and asking for every hit sorted calls a perfect run out of order.
    # Greedy from the left finds a monotone selection whenever one exists,
    # because taking the earliest allowable frame never costs a later flip a
    # choice it would otherwise have had.
    pick, at, prev = [], -1, None
    for f, hits in matched:
        nxt = next((n for n in hits if n > at), None)
        if nxt is None:
            print("     but flip %d has no frame after flip %s's f%05d - the "
                  "sequence differs" % (f, prev, at))
            return 1
        pick.append(nxt)
        at, prev = nxt, f

    print("     port flips %d-%d are hybrid frames %d-%d, in order"
          % (matched[0][0], matched[-1][0], pick[0], pick[-1]))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--screen", action="append",
                    help="a screen to prove; repeatable. Default: all of them")
    ap.add_argument("--seconds", type=int, default=360,
                    help="how long to let the hybrid run")
    ap.add_argument("--port-seconds", type=int, default=400,
                    help="how long to let the port run")
    ap.add_argument("--keep", default=None,
                    help="a directory to leave the frames in, and to reuse "
                         "on a later run - either side already captured there "
                         "is not run again, which is what makes a failure "
                         "cheap to chase. Without it they go to a temporary "
                         "directory and are deleted: a frame is 308 KB and "
                         "this machine's disk has been filled twice")
    args = ap.parse_args()

    screens = [s for s in SCREENS if not args.screen or s[0] in args.screen]
    if not screens:
        raise SystemExit("no such screen - have %s"
                         % ", ".join(repr(s[0]) for s in SCREENS))

    out = args.keep or tempfile.mkdtemp(prefix="check_native")
    pdir, ndir = os.path.join(out, "port"), os.path.join(out, "native")
    os.makedirs(pdir, exist_ok=True)
    os.makedirs(ndir, exist_ok=True)

    try:
        # One run of each side covers every screen: the windows and the flip
        # lists are unions, so a second screen costs comparisons and not
        # another five minutes of emulation.
        window = (min(s[2][0] for s in screens), max(s[2][1] for s in screens))
        want = sorted({f for s in screens for f in s[1]})

        print("hybrid: frames %d-%d ..." % window, flush=True)
        frames = hybrid(window, ndir, args.seconds)
        if not frames:
            raise SystemExit("the hybrid runner produced no frames - it may "
                             "not have reached frame %d in %ds"
                             % (window[0], args.seconds))
        print("hybrid: %d frames" % len(frames))

        print("port: flips %d-%d ..." % (want[0], want[-1]), flush=True)
        ref = reference(want, pdir, args.port_seconds)

        bad = 0
        index = index_of(frames)
        for name, flips, _window, least in screens:
            got = {f: ref[f] for f in flips if f in ref}
            bad += check(name, got, frames, index, least, args.port_seconds)
    finally:
        if not args.keep:
            shutil.rmtree(out, ignore_errors=True)

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
