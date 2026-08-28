"""Read the resource archive, and extract subfiles.

The format is in docs/resources.md and was verified against the bytes there:
`RESOURCE.MAP` indexes the data files, but the data files also walk linearly on
their own, which is what this uses - a subfile is a 13-byte name field, a
UINT32LE length, and the content.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

DATA_FILES = ("RESOURCE.001", "RESOURCE.002", "RESOURCE.003", "RESOURCE.004")


def index():
    """[(name, data file, offset, length)] for every subfile, in file order."""
    out = []
    for fn in DATA_FILES:
        path = os.path.join(tim.GAME_DIR, fn)
        d = open(path, "rb").read()
        off = 0
        while off + 17 <= len(d):
            name = d[off:off + 13].split(b"\0")[0].decode("latin1")
            ln, = struct.unpack("<I", d[off + 13:off + 17])
            out.append((name.upper(), fn, off + 17, ln))
            off += 17 + ln
        if off != len(d):
            raise SystemExit("%s does not walk to its own size (%d/%d)"
                             % (fn, off, len(d)))
    return out


def read(name):
    name = name.upper()
    for nm, fn, off, ln in index():
        if nm == name:
            with open(os.path.join(tim.GAME_DIR, fn), "rb") as f:
                f.seek(off)
                return f.read(ln)
    raise KeyError(name)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--extract", default="", metavar="NAME")
    ap.add_argument("--all", default="", metavar="DIR")
    args = ap.parse_args()
    if args.list:
        for nm, fn, off, ln in index():
            print("%-14s %s @%-8d %7d" % (nm, fn, off, ln))
    if args.extract:
        d = read(args.extract)
        sys.stdout.buffer.write(d)
    if args.all:
        os.makedirs(args.all, exist_ok=True)
        for nm, fn, off, ln in index():
            open(os.path.join(args.all, nm), "wb").write(read(nm))
        print("extracted %d subfiles to %s" % (len(index()), args.all))


if __name__ == "__main__":
    main()
