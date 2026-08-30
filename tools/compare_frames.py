"""Compare the port's frames against the original's, one for one.

`tools/compare_port.py` answers "is the frame the port stopped on right?".
This answers the harder question: does the port stay in step for a whole
screen? It runs the port with `TIM_FRAMES` so every presented frame is written
out, and then, for each captured flip of the original, finds the port frame
closest to it.

The port and the original do not present at the same moments - the original's
captures come one per page flip and the port's one per refresh - so the two are
matched by content. A flip whose best match is exact is one the port drew
right; a flip whose best match is poor is a real difference, and the frame
number it matched at says whether the port is ahead, behind, or diverging.

Indices, never colours: the palette is compared separately, because a frame
that is right and a screen that is black is a palette fault and not a drawing
one.

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
W, H = 640, 400


def run_port(outdir, timeout):
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", TIM_FRAMES=outdir)
    try:
        subprocess.run([os.path.join(ROOT, "reconstruct", "tim")],
                       cwd=ROOT, env=env, capture_output=True, text=True,
                       timeout=timeout)
    except subprocess.TimeoutExpired:
        pass                       # the intro loops; a timeout is the normal end


def load_flip(path):
    d = open(path, "rb").read()
    if d[:8] != b"TIMSCRN1":
        raise SystemExit("%s is not a capture" % path)
    w, h, _ = struct.unpack_from("<HHH", d, 8)
    return w, h, d[-(w * h):]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ref", default=os.path.join(ROOT, "out", "ref"))
    ap.add_argument("--frames", default=None,
                    help="a directory of port frames to use instead of running")
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--step", type=int, default=1,
                    help="compare every Nth captured flip")
    args = ap.parse_args()

    tmp = None
    if args.frames:
        outdir = args.frames
    else:
        tmp = tempfile.mkdtemp(prefix="timframes")
        outdir = tmp
        print("running the port ...")
        run_port(outdir, args.timeout)

    ports = sorted(glob.glob(os.path.join(outdir, "*.raw")))
    if not ports:
        raise SystemExit("the port wrote no frames to %s" % outdir)

    # `TIM_FLIPS` names each frame by the flip it was composed for, which is
    # the same number the captures carry. Then there is nothing to match: flip
    # N is flip N, the comparison is one line per flip, and a run that ended
    # early has no file rather than a stale best match that reads as a
    # difference.
    by_flip = {}
    if all(os.path.basename(p).startswith("flip") for p in ports):
        by_flip = {int(os.path.basename(p)[4:-4]): p for p in ports}
        print("%d port frames, numbered by flip" % len(ports))
    else:
        print("%d port frames" % len(ports))

    frames = [] if by_flip else [open(p, "rb").read() for p in ports]
    ints = [int.from_bytes(f, "big") for f in frames]

    flips = sorted(glob.glob(os.path.join(args.ref, "*.scrn")))[::args.step]
    exact = 0

    missing = 0

    for path in flips:
        w, h, ref = load_flip(path)
        ref = ref[:W * H] if (w, h) != (W, H) else ref
        if len(ref) != W * H:
            # A 640x480 capture: take the top 640x400, which is what the port
            # composes and what the game actually programs the CRTC for.
            ref = b"".join(ref[y * w:y * w + W] for y in range(H))

        if by_flip:
            n = int(os.path.basename(path)[4:8])
            if n not in by_flip:
                missing += 1
                print("  %-16s the port never reached this flip"
                      % os.path.basename(path))
                continue
            got = open(by_flip[n], "rb").read()[:W * H]
            # The count is 256000 interpreted steps; the compare is one
            # memcmp, and most flips are expected to be equal.
            differ = 0 if got == ref else sum(a != b for a, b in zip(got, ref))
            if differ == 0:
                exact += 1
            print("  %-16s %7d of %d differ (%5.2f%%)%s"
                  % (os.path.basename(path), differ, W * H,
                     100.0 * differ / (W * H),
                     "   <- exact" if differ == 0 else ""))
            continue

        # An exact match is a bytes compare, which is one memcmp.
        best, best_at = None, -1
        for i, f in enumerate(frames):
            if f == ref:
                best, best_at = 0, i
                break

        # Nothing exact, so the closest frame has to be found. Counting
        # differing *bytes* in Python is 256000 interpreted steps per frame,
        # and over a screen's worth of flips that runs for hours; XOR-ing the
        # two frames as one big integer and counting the set bits is the same
        # walk done in C. Differing bits rank a little differently from
        # differing bytes, so the ranking only picks the frame - the byte
        # count is then paid once, for that frame alone, and is what is
        # reported.
        if best is None:
            ref_i = int.from_bytes(ref, "big")
            best_at = min(range(len(frames)),
                          key=lambda i: (ints[i] ^ ref_i).bit_count())
            best = sum(a != b for a, b in zip(frames[best_at], ref))

        if best == 0:
            exact += 1
        print("  %-16s best port frame %5d  %7d of %d differ (%5.2f%%)%s"
              % (os.path.basename(path), best_at, best, W * H,
                 100.0 * best / (W * H), "   <- exact" if best == 0 else ""))

    print("\n%d of %d captured flips matched exactly%s"
          % (exact, len(flips),
             " (%d the port never reached)" % missing if missing else ""))

    if tmp:
        print("(port frames left in %s)" % tmp)


if __name__ == "__main__":
    main()
