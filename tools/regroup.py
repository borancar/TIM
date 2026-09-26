# SPDX-License-Identifier: GPL-2.0-only
"""**Gather a module's routines into its file**, in address order.

    uv run python tools/regroup.py LO HI TARGET.c [--dry-run]

Every function definition in the port whose provenance address is in
[LO, HI) is cut out of the file it is in - with the comment block directly
above it, which is its provenance - and inserted into TARGET.c, which keeps
everything else it had; the moved and the already-present routines of the
range end up in address order, after TARGET's last non-range routine or at
its end. File-scope objects are not moved: a record only these routines use
has to be moved by hand, and the build says which.

This file is the port's own tooling, not a transcription.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))), "reconstruct", "tests"))
import judge          # noqa: E402
import provenance     # noqa: E402
from cparse import parse, text   # noqa: E402

ADDR = re.compile(r"(?:[0-9a-fA-F]{4}:[0-9a-fA-F]{4},\s*image\s+)?"
                  r"(0x[0-9a-fA-F]{5})\b")


def definitions(path):
    """(address, start byte, end byte) of each addressed definition, the
    start taken from its comment block - and the file's own bytes. The parse
    is of cparse's expansion, which keeps every offset but blanks macros, so
    the offsets are used on the raw text and the expansion is never written
    back."""
    _expanded, root = parse(path)
    src = open(path, "rb").read()
    out = []
    for node in root.children:
        if node.type != "function_definition":
            continue
        block = provenance.comment_above(node)
        if block is None:
            continue
        m = ADDR.match(provenance.first_content_line(text(_expanded, block)))
        if m:
            out.append((int(m.group(1), 16), block.start_byte, node.end_byte))
    return src, out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("lo", type=lambda v: int(v, 16))
    ap.add_argument("hi", type=lambda v: int(v, 16))
    ap.add_argument("target")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args(argv)
    target = os.path.abspath(a.target)
    moved = []
    edits = {}
    for path in judge.port_sources():
        src, defs = definitions(path)
        cut = [(s, e, addr) for addr, s, e in defs if a.lo <= addr < a.hi]
        if not cut:
            continue
        for s, e, addr in cut:
            moved.append((addr, src[s:e].decode("utf-8"),
                          os.path.basename(path)))
        keep, pos = [], 0
        for s, e, _ in sorted(cut):
            keep.append(src[pos:s])
            pos = e
            # and the blank line after it
            while pos < len(src) and src[pos:pos + 1] == b"\n":
                pos += 1
        keep.append(src[pos:])
        edits[path] = b"\n".join(x.rstrip(b"\n") + b"\n" for x in keep
                                 if x.strip()) if False else b"".join(keep)
    moved.sort()
    for addr, _, where in moved:
        print("%05x  from %s" % (addr, where))
    if a.dry_run:
        return 0
    for path, data in edits.items():
        if os.path.abspath(path) != target:
            open(path, "wb").write(data)
    base = edits.get(target) if target in edits else (
        open(target, "rb").read() if os.path.exists(target) else b"")
    body = base.rstrip(b"\n") + b"\n\n" + "\n\n".join(
        t for _, t, _ in moved).encode("utf-8") + b"\n"
    open(target, "wb").write(body)
    print("%d routines into %s" % (len(moved), a.target))
    return 0


if __name__ == "__main__":
    sys.exit(main())
