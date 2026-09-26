# SPDX-License-Identifier: GPL-2.0-only
"""**Where the image's modules begin and end**, from how its far calls are
encoded.

    uv run python tools/modmap.py [--segment 0x248f] [-v]

This file is the port's own tooling, not a transcription.

In the medium model several source files can share one code segment - they
were compiled into the same segment name and TLINK laid them end to end. A
segment boundary is a module boundary, but a segment can hold several
modules, and the boundaries inside it have to be read off the bytes. Two
encodings of a far call into the caller's own segment say which side of a
boundary a callee is on:

- `push cs / call near` with nothing in front is Borland C++'s own: it emits
  that for a routine **defined earlier in the same file**. Caller and callee
  are one module, so no boundary lies between them.
- `nop / push cs / call near` is TLINK's rewrite of a `9A`: the compiler did
  not know the callee as defined earlier in its file. If the callee is
  *earlier* in the image all the same, a boundary lies between the two.

Each segment's routines come from the port's own provenance (the addresses
`tools/judge.py` indexes). The answer is a list of stretches that must lie in
one module, and gaps where a boundary must fall - somewhere between two
routines, not necessarily at a routine the port has.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import judge    # noqa: E402  the address index, the image and its frames


def calls(img, lo, hi):
    """Every far call into the segment [lo, hi): (site, target, has nop)."""
    out = []
    for a in range(lo, hi - 3):
        if img[a] == 0x0E and img[a + 1] == 0xE8:
            rel = struct.unpack_from("<h", img, a + 2)[0]
            base = lo
            tgt = base + ((a + 4 - base + rel) & 0xFFFF)
            if lo <= tgt < hi:
                out.append((a - 1 if img[a - 1] == 0x90 else a, tgt,
                            img[a - 1] == 0x90))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--segment", type=lambda v: int(v, 16), default=None,
                    help="only this frame (paragraph, e.g. 248f)")
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)
    img = open(judge.IMAGE, "rb").read()
    known = judge.addresses(judge.port_sources())
    fr = judge.frames() + [0x2D3C0]
    names = {v: k for k, v in known.items()}
    for f, f_end in zip(fr, fr[1:]):
        if a.segment is not None and f != a.segment * 16:
            continue
        routines = sorted(x for x in names if f <= x < f_end)
        if not routines:
            continue

        def owner(addr):
            r = [x for x in routines if x <= addr]
            return r[-1] if r else None
        same, apart = [], []
        for site, tgt, nop in calls(img, f, f_end):
            src = owner(site)
            if src is None or tgt not in names:
                continue
            if not nop and tgt < site:
                same.append((tgt, src))
            elif nop and tgt < src:
                apart.append((tgt, src))
        # merge "same" spans; a boundary cannot fall strictly inside one
        spans = sorted(same)
        merged = []
        for lo_, hi_ in spans:
            if merged and lo_ <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], hi_))
            else:
                merged.append((lo_, hi_))
        # the smallest "apart" windows, less what a same-span covers
        need = []
        for lo_, hi_ in sorted(apart, key=lambda p: p[1] - p[0]):
            gaps = [x for x in routines if lo_ < x <= hi_
                    and not any(s < x <= e for s, e in merged)]
            if gaps and not any(set(gaps) >= set(g) for _, g in need):
                need.append(((lo_, hi_), gaps))
        print("segment %04x  %05x..%05x  %d routines, %d same-file calls, "
              "%d nop calls to earlier routines"
              % (f >> 4, f, f_end, len(routines), len(same), len(apart)))
        for lo_, hi_ in merged:
            print("   one module: %05x %s .. %05x %s"
                  % (lo_, names[lo_], hi_, names[hi_]))
        cuts = set()
        for (lo_, hi_), gaps in sorted(need):
            if any(g in cuts for g in gaps):
                continue
            if len(gaps) == 1:
                cuts.add(gaps[0])
            if a.verbose or len(gaps) == 1:
                print("   boundary %s: before one of %s (%s -> %s)"
                      % ("fixed" if len(gaps) == 1 else "somewhere",
                         " ".join("%05x" % g for g in gaps),
                         names[hi_], names[lo_]))
        print("   fixed boundaries before: %s"
              % (" ".join("%05x %s" % (c, names[c]) for c in sorted(cuts))
                 or "none"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
