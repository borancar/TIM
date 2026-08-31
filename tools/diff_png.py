"""Compare a capture against the port's output, and say what differs.

A percentage says how much is wrong and never says *what*, so this always
writes three images and expects them to be looked at:

    <name>-original.png   what the emulator's screen actually held
    <name>-port.png       what the port composed
    <name>-diff.png       the port, every differing pixel in magenta and
                          everything that agrees dimmed to a quarter

Each side is rendered through **its own palette**, so a palette error shows as
a colour difference instead of silently cancelling. "Differs" is decided on the
palette *index*, never on RGB - two different indices can share a colour, and
that is exactly the class of bug worth seeing.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png
from capture import read_scrn

MAGENTA = (255, 0, 255)


def load_raw(path, w, h):
    """The port's frame - a `TIMSCRN1` container, or bare indices.

    `TIM_FLIPS` writes the same container the captures use, so a port frame
    carries its own palette and its own blanking line and nothing here has to
    supply them. Bare indices are still accepted because `tools/compare_port.py`
    writes one.
    """
    d = open(path, "rb").read()
    if d[:8] == b"TIMSCRN1":
        return read_scrn(path)[4][:w * h]
    if len(d) < w * h:
        raise SystemExit("%s holds %d bytes, need %d for %dx%d"
                         % (path, len(d), w * h, w, h))
    return d[:w * h]


def compare(cap_path, raw_path, name, rows=None, cols=None, scale=1,
            port_palette=None):
    w, h, svb, pal, ref = read_scrn(cap_path)

    # A capture taken while the CRTC is still set for 480 lines is 640x480; the
    # port composes the 640x400 the game actually programs. Take the top of the
    # capture rather than refusing, which is what `tools/compare_frames.py`
    # does too - the rows below are not part of the frame either side drew.
    if (open(raw_path, "rb").read(8) != b"TIMSCRN1"
            and os.path.getsize(raw_path) == w * 400 and h != 400):
        ref = ref[:w * 400]
        h = 400

    got = load_raw(raw_path, w, h)
    ppal = port_palette or pal

    y0, y1 = rows if rows else (0, h)
    x0, x1 = cols if cols else (0, w)

    differ = 0
    total = 0
    for y in range(y0, y1):
        base = y * w
        for x in range(x0, x1):
            total += 1
            if ref[base + x] != got[base + x]:
                differ += 1

    print("  region %dx%d at (%d,%d)" % (x1 - x0, y1 - y0, x0, y0))
    print("  differing : %d of %d  (%.2f%% agree)"
          % (differ, total, 100.0 * (total - differ) / max(1, total)))

    png.save_indexed(name + "-original.png", ref, w, h, pal,
                     x0=x0, x1=x1, y0=y0, y1=y1, scale=scale)
    png.save_indexed(name + "-port.png", got, w, h, ppal,
                     x0=x0, x1=x1, y0=y0, y1=y1, scale=scale)

    # The diff: agreement dimmed to a quarter, disagreement in a colour the
    # game's own palette cannot produce.
    dim = [(r // 4, g // 4, b // 4) for (r, g, b) in ppal]
    drows = []
    for y in range(y0, y1):
        row = bytearray()
        base = y * w
        for x in range(x0, x1):
            c = MAGENTA if ref[base + x] != got[base + x] else dim[got[base + x]]
            row += bytes(c) * scale
        for _ in range(scale):
            drows.append(row)
    png.write_png(name + "-diff.png", (x1 - x0) * scale, (y1 - y0) * scale, drows)

    for suffix in ("-original.png", "-port.png", "-diff.png"):
        print("    " + name + suffix)
    return differ, total


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--capture", required=True, help="a .scrn from tools/capture.py")
    ap.add_argument("--raw", required=True, help="the port's 8-bit index buffer")
    ap.add_argument("--name", required=True, help="output prefix")
    ap.add_argument("--rows", nargs=2, type=int, default=None, metavar=("Y0", "Y1"))
    ap.add_argument("--cols", nargs=2, type=int, default=None, metavar=("X0", "X1"))
    ap.add_argument("--scale", type=int, default=1)
    args = ap.parse_args()
    differ, _ = compare(args.capture, args.raw, args.name,
                        rows=args.rows, cols=args.cols, scale=args.scale)
    return 0 if differ == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
