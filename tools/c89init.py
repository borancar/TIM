#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""**Rewrites C99 designated initialisers as Borland C++ reads them.**

The port writes DGROUP's tables with designators - `.frame = { [1] = 0xff }`,
`[KIND_RAMP] = { .density = ... }` - which gcc takes and Borland C++, a C89
compiler, does not. clang resolves every initialiser list to its positional
form: designators applied, enum indices evaluated, and each member left out
made explicit. This reads that form from clang's JSON AST and writes it back
as C89, each leaf in its source's own spelling (a function name stays a
function name, a string stays a string), a member left out as `0` - every
member written, so gcc has none to warn about - and each
struct member's name in a comment, so the table still reads.

    uv run python tools/c89init.py reconstruct/src/dgroup.c NAME [NAME ...]

prints the definitions, with the positional initialisers in place of the
designated ones. The declarator - type, name and the rest - is
kept as written. `--spans` prints JSON instead: each definition's new text
and the byte range it replaces in the file.

This file is the port's own tooling, not a transcription.
"""
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from version import REPO, RECON  # noqa: E402


def ast(path):
    r = subprocess.run(
        ["clang", "-Xclang", "-ast-dump=json", "-fsyntax-only", "-std=gnu11",
         "-I", RECON, "-I", os.path.dirname(path),
         "-Wno-everything", path],
        capture_output=True, text=True)
    return json.loads(r.stdout)


def records(root):
    """Every struct/union definition, by its type name, with its fields in
    order."""
    out = {}

    def walk(n):
        if n.get("kind") == "RecordDecl" and n.get("completeDefinition"):
            fields = [(f.get("name", ""), f["type"]["qualType"])
                      for f in n.get("inner", []) if f.get("kind") == "FieldDecl"]
            if n.get("name"):
                out[n["tagUsed"] + " " + n["name"]] = fields
        for c in n.get("inner", []):
            walk(c)
    walk(root)
    return out


class Writer:
    def __init__(self, src, recs):
        self.src = src
        self.recs = recs

    def text(self, n):
        r = n["range"]
        b, e = r["begin"], r["end"]
        b = b.get("spellingLoc", b)
        e = e.get("spellingLoc", e)
        if "offset" not in b or "offset" not in e:
            return None
        return self.src[b["offset"]:e["offset"] + e.get("tokLen", 0)]

    def zero(self, qt):
        """A zero initialiser braced as the type nests, which is what gcc's
        -Wmissing-braces asks: `{ { 0 } }` for an array of structs."""
        qt = qt.replace("const ", "").replace("volatile ", "").strip()
        if qt.endswith("]"):
            return "{ %s }" % self.zero(qt[:qt.rindex("[")].strip())
        fields = self.recs.get(qt)
        if fields:
            return "{ %s }" % self.zero(fields[0][1])
        return "0"

    def leaf(self, n):
        k = n["kind"]
        if k == "ImplicitValueInitExpr":
            return "0"
        if k in ("ImplicitCastExpr",) and n.get("inner"):
            t = self.text(n)
            return t if t is not None else self.leaf(n["inner"][0])
        t = self.text(n)
        if t is None:
            raise SystemExit("no source text for %s" % k)
        return t

    def value(self, n, indent):
        if n["kind"] == "InitListExpr":
            return self.init_list(n, indent)
        if n["kind"] == "ImplicitValueInitExpr":
            return self.zero(n["type"]["qualType"])
        return self.leaf(n)

    def init_list(self, n, indent):
        qt = n["type"]["qualType"]
        # an array whose tail is filled has its elements under
        # `array_filler`, after the filler itself (zero, which C89 fills in)
        if "array_filler" in n:
            items = n["array_filler"][1:]
        else:
            items = n.get("inner", [])
        fields = self.recs.get(qt.split(" [")[0]) if not qt.endswith("]") else None
        vals = [self.value(c, indent + 1) for c in items]
        # every member said, so nothing is left for C to fill in - except a
        # wholly zero aggregate, written as its type's zero
        if all(re.fullmatch(r"[{ ]*0[} ]*", v) for v in vals):
            return self.zero(qt)
        one = "{ " + ", ".join(vals) + " }"
        if len(one) + 4 * indent <= 76 and "\n" not in one:
            return one
        pad = "    " * (indent + 1)
        lines = []
        for i, v in enumerate(vals):
            note = ""
            if fields and i < len(fields) and fields[i][0]:
                note = "    /* %s */" % fields[i][0]
            lines.append(pad + v + ("," if i < len(vals) - 1 else "") + note)
        return "{\n" + "\n".join(lines) + "\n" + "    " * indent + "}"


def main(argv):
    spans = "--spans" in argv
    argv = [a for a in argv if a != "--spans"]
    path, names = argv[0], set(argv[1:])
    src = open(path).read()
    root = ast(path)
    w = Writer(src, records(root))
    found = {}
    for n in root.get("inner", []):
        if n.get("kind") != "VarDecl" or n.get("name") not in names:
            continue
        if not n.get("inner"):
            continue
        inits = [c for c in n["inner"] if c.get("kind", "").endswith("Expr")]
        if not inits:
            continue
        init = inits[-1]
        rng = n["range"]
        begin = rng["begin"].get("spellingLoc", rng["begin"])["offset"]
        ib = init["range"]["begin"].get("spellingLoc", init["range"]["begin"])["offset"]
        head = src[begin:ib].rstrip()
        end = src.index(";", init["range"]["end"].get("spellingLoc", init["range"]["end"])["offset"]) + 1
        found[n["name"]] = ("%s %s;" % (head, w.value(init, 0)), begin, end)
    missing = names - set(found)
    if missing:
        raise SystemExit("not found with an initialiser: %s" % ", ".join(sorted(missing)))
    if spans:
        print(json.dumps({k: {"text": v[0], "begin": v[1], "end": v[2]}
                          for k, v in found.items()}))
        return
    for n in argv[1:]:
        print(found[n][0])
        print()


if __name__ == "__main__":
    main(sys.argv[1:])
