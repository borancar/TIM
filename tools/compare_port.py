"""Run the port and compare the frame it stops on against the original's.

The port stops at the first routine it has not got yet and writes the composed
frame out as palette **indices**; tools/capture.py takes the original's frames
on the guest's own page-flip cue, in the same form. This puts the two beside
each other.

It compares against *every* captured flip and reports the closest, because the
port and the original are not at the same point: the port stops where its next
gap is, and which flip that lands between is exactly what one wants told rather
than assumed. An exact match against some flip is the result worth having; a
near miss against one is a difference worth looking at; a poor match against all
of them usually means the port stopped before the screen was finished, not that
it drew it wrongly.

Indices, never colours: two different indices can share a colour, and a
comparison that came down to RGB could not see the difference.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 640, 400

EGA = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
       (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
       (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
       (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255)]


def run_port(out):
    """Run the game binary until it stops, and answer the frame it wrote."""
    # `devtim`, which is where the developer flags live: the Makefile's rule is
    # that none of them may reach what ships. It has no window, so there is no
    # dummy video driver to arrange and nothing to hold open - it writes the
    # frame from its abort hook and exits.
    env = dict(os.environ, TIM_FRAME=out)
    p = subprocess.run([os.path.join(ROOT, "reconstruct", "devtim")],
                       cwd=ROOT, env=env, capture_output=True, text=True,
                       timeout=600)
    why = ""
    for line in (p.stderr or "").splitlines():
        if "not transcribed" in line:
            why = line.strip()
    if not os.path.exists(out):
        raise SystemExit("the port wrote no frame:\n" + (p.stderr or "")[-800:])
    return open(out, "rb").read(), why


def load_scrn(path):
    d = open(path, "rb").read()
    if d[:8] != b"TIMSCRN1":
        raise SystemExit("%s is not a capture" % path)
    w, h, svb = struct.unpack_from("<HHH", d, 8)
    return w, h, d[-(w * h):]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ref", default=os.path.join(ROOT, "out", "ref"),
                    help="directory of tools/capture.py output")
    ap.add_argument("--out", default=os.path.join(ROOT, "out", "port.raw"))
    ap.add_argument("--png", default=None,
                    help="also write the port's frame here, for looking at")
    ap.add_argument("-n", type=int, default=5, help="how many flips to list")
    args = ap.parse_args()

    frame, why = run_port(args.out)
    if why:
        print("the port stopped: %s" % why)
    print("its frame: %d bytes, %d distinct indices"
          % (len(frame), len(set(frame))))

    if args.png:
        png.save_indexed(args.png, frame, W, H, EGA + [(0, 0, 0)] * 240)
        print("wrote %s" % args.png)

    caps = sorted(glob.glob(os.path.join(args.ref, "*.scrn")))
    if not caps:
        raise SystemExit("no captures in %s - run tools/capture.py first"
                         % args.ref)

    rows = []
    for p in caps:
        w, h, idx = load_scrn(p)
        ref = idx[:W * H]
        if len(ref) < W * H:
            continue
        rows.append((sum(1 for a, b in zip(frame, ref) if a != b),
                     os.path.basename(p)))
    rows.sort()

    for diff, name in rows[:args.n]:
        print("  %-16s %7d of %d differ  (%6.2f%%)%s"
              % (name, diff, W * H, 100.0 * diff / (W * H),
                 "   <- exact" if diff == 0 else ""))

    return 0 if rows and rows[0][0] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
