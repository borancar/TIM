"""Prove the hybrid runner draws what the port draws.

`tools/native/` runs the original binary under emulation with the port standing
in for the hardware and, routine by routine, for the code. This asks whether
what it *draws* is what the port draws - which is the only question that
matters about a graphics layer, and the one a run that merely fails to crash
does not answer.

**The two runs cannot be compared flip by flip.** They present frames at very
different rates: the port reaches the title screen by its sixth flip, and the
hybrid takes several hundred, because their timer pacing is not the same. A
frame-numbered comparison of the two says 83% of pixels differ and means
nothing at all - it is comparing different moments. That mistake cost an
afternoon here, so this aligns by *content* instead: it asks whether any frame
the hybrid produces is byte for byte the reference, which is a claim about the
drawing and not about the clock.

**The title screen animates**, so matching it means finding a pair, not a
frame: the port's own consecutive flips there differ by six hundred to eleven
hundred pixels, and a single sampled frame differed by 1,429 for no better
reason than being a different moment in the same animation. Sweeping the
hybrid's frames against the reference finds the phase that agrees.

The reference is a `.scrn` from `devtim`, the same capture `check_briefing.py`
compares against, so both sides are the port's own `vga_compose` and palette.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# The intro's two logo screens live in one 480-line buffer on two pages: the
# Sierra logo in the displayed rows and Jeff Tunnell Productions lower down.
# Named so a difference can be reported against the screen it is in rather than
# as a pixel count over the whole frame.
REGIONS = (("Sierra logo",   0, 400),
           ("Jeff Tunnell", 425, 470))


def need(path, how):
    if not os.path.exists(path):
        raise SystemExit("no %s - %s" % (os.path.relpath(path, ROOT), how))
    return path


def reference(flip, outdir):
    """One flip out of the port, as `check_briefing.py` would take it."""
    devtim = need(os.path.join(ROOT, "reconstruct", "devtim"),
                  "run `make -C reconstruct devtim`")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy",
               TIM_FLIPWANT=str(flip),
               TIM_FLIPS="%s:%d" % (outdir, flip))
    try:
        subprocess.run([devtim], cwd=ROOT, env=env, timeout=120,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        pass                       # neither side exits: a DOS game does not
    path = os.path.join(outdir, "flip%04d.scrn" % flip)
    if not os.path.exists(path):
        raise SystemExit("the port did not reach flip %d" % flip)
    d = open(path, "rb").read()
    w, h, _vis = struct.unpack_from("<HHH", d, 8)
    return d[14:14 + 768], d[-(w * h):]


def hybrid(step, outdir, seconds):
    """Every step-th frame out of the hybrid runner."""
    native = need(os.path.join(ROOT, "tools", "native", "native"),
                  "run `make -C tools/native`")
    env = dict(os.environ, TIM_FRAMES="%s:%d" % (outdir, step))
    try:
        subprocess.run([native], cwd=ROOT, env=env, timeout=seconds,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        pass                       # a DOS game does not exit; that is the plan
    return sorted(glob.glob(os.path.join(outdir, "*.raw")))


def compare(ref_pal, ref_idx, path):
    d = open(path, "rb").read()
    pal, idx = d[:768], d[768:]
    if len(idx) != len(ref_idx):
        return None
    return (sum(1 for a, b in zip(ref_idx, idx) if a != b),
            sum(1 for a, b in zip(ref_pal, pal) if a != b), idx, pal)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--flip", type=int, action="append",
                    help="a port flip to match; repeatable. The default pair "
                         "is 4 (both intro logos) and 64 (the title screen)")
    ap.add_argument("--step", type=int, default=30,
                    help="dump every step-th hybrid frame")
    ap.add_argument("--seconds", type=int, default=60)
    ap.add_argument("--keep", default=None)
    args = ap.parse_args()
    flips = args.flip or [4, 64]

    out = args.keep or tempfile.mkdtemp(prefix="native")
    pdir, ndir = os.path.join(out, "port"), os.path.join(out, "native")
    os.makedirs(pdir, exist_ok=True)
    os.makedirs(ndir, exist_ok=True)

    print("hybrid: running ...", flush=True)
    frames = hybrid(args.step, ndir, args.seconds)
    if not frames:
        raise SystemExit("the hybrid runner produced no frames")
    print("hybrid: %d frames" % len(frames))

    bad = 0
    for flip in flips:
        print("\nport: capturing flip %d ..." % flip, flush=True)
        ref_pal, ref_idx = reference(flip, pdir)

        best = None
        for path in frames:
            r = compare(ref_pal, ref_idx, path)
            if r and (best is None or r[0] + r[1] < best[0] + best[1]):
                best = (r[0], r[1], path, r[2])
            if r and r[0] == 0 and r[1] == 0:
                break

        diff, paldiff, path, idx = best
        name = os.path.basename(path)[:-4]
        if diff == 0 and paldiff == 0:
            print("  **%s is byte for byte the port's flip %d** - %d pixels "
                  "and 768 palette bytes identical" % (name, flip, len(ref_idx)))
        else:
            bad += 1
            print("  closest is %s: %d of %d pixels differ, %d of 768 palette "
                  "bytes" % (name, diff, len(ref_idx), paldiff))
            for what, y0, y1 in REGIONS:
                d = sum(1 for a, b in zip(ref_idx[y0 * 640:y1 * 640],
                                          idx[y0 * 640:y1 * 640]) if a != b)
                print("      %-14s rows %3d-%3d : %d differ"
                      % (what, y0, y1 - 1, d))

    if not args.keep:
        print("\n(frames in %s)" % out)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
