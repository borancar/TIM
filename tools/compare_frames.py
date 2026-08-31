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

**A whole run is compared by digest, not by keeping the frames.** `--digests`
takes the two files `tools/capture.py --digests` and the port's `TIM_FLIPHASH`
write - one `flip crc` line each - and names the flips that differ. Keeping
every frame on both sides to prove that none of them differed cost five
gigabytes; the same answer is a few hundred kilobytes. The directory-of-frames
path below is for the handful of flips a side-by-side actually needs.

**The whole 640x480, and this file used to crop it.** An earlier version took
the top 640x400 of each capture, with a comment saying that was what the game
programs the CRTC for. That is true of the intro screens and false of the
Sierra logo, which asks for 470 rows - so the comparison could not see the
bottom seventy rows of the logo, the port composed only 400 and cut them, and
"every captured flip matches exactly" was reported over a frame that was missing
the part that differed. An instrument that shares the port's blind spot cannot
report it. A capture that is not the size expected is now an error, not a crop.

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
W, H = 640, 480


def run_port(outdir, timeout, last=None):
    """Run the port, writing one frame per page flip, named by the flip.

    `devtim`, not `tim`: the developer flags live there and the Makefile's rule
    is that none of them may reach what ships. And `TIM_FLIPS` rather than the
    `TIM_FRAMES` this used to use - a presented frame had to be matched to a
    capture by content, because presents and page flips do not line up, whereas
    a flip-numbered frame needs no matching at all. That is what the rest of
    this file already prefers; the by-content path was the fallback.
    """
    env = dict(os.environ,
               TIM_FLIPS=outdir if last is None else "%s:%d" % (outdir, last))
    try:
        subprocess.run([os.path.join(ROOT, "reconstruct", "devtim")],
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


def compare_digests(ref_path, port_path):
    """Compare two digest files, flip by flip, and name the flips that differ.

    This is the whole-run comparison. Neither side keeps a pixel: a run is a few
    hundred kilobytes of `flip crc` lines, and what comes out is the list of
    flips worth looking at. Take those with `tools/capture.py --flip N` and the
    port's `TIM_FLIPS=<dir>:<N>`, and diff the two frames.
    """
    def read(path):
        out = {}
        for line in open(path):
            parts = line.split()
            if len(parts) >= 2:
                out[int(parts[0])] = parts[1].lower()
        return out

    ref, port = read(ref_path), read(port_path)
    common = sorted(set(ref) & set(port))
    if not common:
        raise SystemExit("no flip appears in both files")

    differ = [n for n in common if ref[n] != port[n]]
    only_ref = sorted(set(ref) - set(port))
    only_port = sorted(set(port) - set(ref))

    for n in differ[:40]:
        print("  flip %-6d differs  (original %s, port %s)"
              % (n, ref[n], port[n]))
    if len(differ) > 40:
        print("  ... and %d more" % (len(differ) - 40))

    print("%d of %d flips in both files match"
          % (len(common) - len(differ), len(common)))
    if only_ref:
        print("%d flips the port never reached (first %d)"
              % (len(only_ref), only_ref[0]))
    if only_port:
        print("%d flips the original was not captured for (first %d)"
              % (len(only_port), only_port[0]))
    if differ:
        print("look at one with:")
        print("  uv run python tools/capture.py --flip %d --out out/one" % differ[0])
        # **`TIM_FLIPWANT`, not just the stop.** `TIM_FLIPS=<dir>:<n>` writes
        # a frame for every flip up to `n`, which is a quarter of a gigabyte by
        # flip 800; naming the one flip wanted writes one file.
        print("  TIM_FLIPWANT=%d TIM_FLIPS=out/portone:%d ./reconstruct/devtim"
              % (differ[0], differ[0]))
        print("  uv run python tools/diff_png.py ...")
    return 1 if differ or only_ref else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--digests", nargs=2, metavar=("REF", "PORT"),
                    default=None,
                    help="compare two digest files instead of two directories "
                         "of frames - the way to check a whole run")
    ap.add_argument("--ref", default=os.path.join(ROOT, "out", "ref"))
    ap.add_argument("--frames", default=None,
                    help="a directory of port frames to use instead of running")
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--step", type=int, default=1,
                    help="compare every Nth captured flip")
    args = ap.parse_args()

    if args.digests:
        return compare_digests(*args.digests)

    tmp = None
    if args.frames:
        outdir = args.frames
    else:
        tmp = tempfile.mkdtemp(prefix="timframes")
        outdir = tmp
        print("running the port ...")
        run_port(outdir, args.timeout)

    ports = sorted(glob.glob(os.path.join(outdir, "*.scrn")))
    if not ports:
        raise SystemExit("the port wrote no frames to %s" % outdir)

    # `TIM_FLIPS` names each frame by the flip it was composed for, which is
    # the same number the captures carry. Then there is nothing to match: flip
    # N is flip N, the comparison is one line per flip, and a run that ended
    # early has no file rather than a stale best match that reads as a
    # difference.
    by_flip = {}
    if all(os.path.basename(p).startswith("flip") for p in ports):
        by_flip = {int(os.path.basename(p)[4:-5]): p for p in ports}
        print("%d port frames, numbered by flip" % len(ports))
    else:
        print("%d port frames" % len(ports))

    frames = [] if by_flip else [load_flip(p)[2] for p in ports]
    ints = [int.from_bytes(f, "big") for f in frames]

    flips = sorted(glob.glob(os.path.join(args.ref, "*.scrn")))[::args.step]
    exact = 0

    missing = 0

    for path in flips:
        w, h, ref = load_flip(path)
        if (w, h) != (W, H):
            raise SystemExit("%s is %dx%d, not %dx%d" % (path, w, h, W, H))

        if by_flip:
            n = int(os.path.basename(path)[4:8])
            if n not in by_flip:
                missing += 1
                print("  %-16s the port never reached this flip"
                      % os.path.basename(path))
                continue
            got = load_flip(by_flip[n])[2]
            # The count is 307200 interpreted steps; the compare is one
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
        # differing *bytes* in Python is 307200 interpreted steps per frame,
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
