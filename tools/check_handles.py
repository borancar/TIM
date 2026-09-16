#!/usr/bin/env python3
"""Refuse a typed handle used as a boolean.

The port's handles - `struct part *`, `struct belt *`, `struct rope *`,
`struct rect_list_entry *` - are views of DGROUP: `PART_PTR(p)` is
`dgroup + p`, so the original's "no part", an offset of 0, is DGROUP:0 and
**never NULL**. Every `*_NONE` sentinel exists for that reason. It follows
that `if (p)`, `p ? ... : ...`, `!p`, `p && ...` and `... || p` on a handle
are always true where the original tested an offset against 0 - and the
compiler accepts every one of them, because a pointer in a boolean context is
legal C.

That is not hypothetical. `part_under_pointer`'s `link ? link->owner_ptr : 0`
landed in e8045fc and passed both builds, the full verification sweep, the
intro byte for byte and all 29 solutions, because the difference needs a
ropeless part and an `exclude` no captured run supplies. See
docs/lessons.md, "A typed handle tested as a boolean is always true". A check
that cannot reach a fault's condition cannot find it; a structural one can.

Every handle compares with its sentinel instead: `p != PART_NONE`.

With no arguments it reads the game and the port both, as CLAUDE.md asks of a
tool that reads the sources; given paths, it reads those. Exits 1 on any hit.
Ours; not a transcription.
"""
import glob
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HANDLE_DECL = re.compile(r"struct (?:part|belt|rope|rect_list_entry) \*\s*(\w+)")


def scan(path):
    hits = []
    lines = open(path).read().split("\n")
    for i, ln in enumerate(lines):
        if not re.match(r"^[a-z_0-9]+[ \*]+\w+\(", ln):
            continue
        j = i
        while j < len(lines) - 1 and lines[j] != "}":
            j += 1
        body = lines[i:j + 1]
        typed = set(HANDLE_DECL.findall("\n".join(body)))
        for k, s in enumerate(body):
            code = re.sub(r"/\*.*?\*/|//.*$", "", s)
            for v in typed:
                V = re.escape(v)
                if (re.search(r"(?<![\w>.])%s\s*\?" % V, code)
                        or re.search(r"\b(?:if|while)\s*\(\s*!?\s*%s\s*\)" % V, code)
                        or re.search(r"(?<![\w>.!=<])!\s*%s\b(?!\s*(?:->|\.|\[))" % V, code)
                        or re.search(r"(?:^|[(,?:]|&&|\|\||\breturn|(?<![=!<>])=)\s*%s\s*(?:&&|\|\|)" % V, code)
                        or re.search(r"(?:&&|\|\|)\s*!?\s*%s\s*(?:\)|&&|\|\||$)" % V, code)):
                    hits.append((i + k + 1, v, code.strip()))
    return hits


def main():
    paths = sys.argv[1:] or (sorted(glob.glob(os.path.join(REPO, "reconstruct", "src", "*.c")))
                             + sorted(glob.glob(os.path.join(REPO, "reconstruct", "*.c"))))
    # **A scan of nothing is not a pass.** Run from anywhere but tools/, the
    # default glob found no sources and this printed its all-clear over zero
    # files - which is how a copy under test "accepted" a tree it never read.
    if not paths:
        print("FAIL: no sources found to scan under %s" % REPO)
        return 1
    bad = 0
    for p in paths:
        for line, v, code in scan(p):
            print("FAIL %s:%d: handle `%s` used as a boolean: %s"
                  % (os.path.relpath(p, REPO), line, v, code[:80]))
            bad += 1
    if not bad:
        print("no typed handle is used as a boolean")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
