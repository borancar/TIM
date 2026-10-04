# SPDX-License-Identifier: GPL-2.0-only
"""**Designated initialisers made positional**, for a compiler that has none.

    uv run python tools/positional.py FILE.c [FILE.c ...] [--check]

Turbo C++ 3.0 is C89: `{ .stop = 0xffff, .stop_x = { 0x00e8, 0x0168 } }`
and `{ [1] = 5 }` are syntax errors to it. This rewrites every designated
initialiser in the files named as the positional one it means, in the
struct's own field order, filling what was left out with `0` (or `{0}` for
an aggregate) up to the last member given - the bytes are the same, and the
original's source was written this way. Each member of a rewritten struct
initialiser keeps its name in a comment.

Struct layouts come from the port's headers and the file itself, over
cparse's parse. A union, or a type it cannot resolve, is left as it is and
reported, so nothing is guessed. `--check` only reports what it would do.

This file is the port's own tooling, not a transcription.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cparse import parse, text     # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
from version import RECON  # noqa: E402
HEADERS = [os.path.join(RECON, h) for h in ("dgroup.h", "tim.h", "hostio.h")]


class Unresolved(Exception):
    pass


def struct_defs(paths):
    """struct name -> [(field, type)], a type being ('struct', name),
    ('array', type) or ('scalar',)."""
    out = {}
    for path in paths:
        src, root = parse(path)
        stack = [root]
        while stack:
            n = stack.pop()
            stack.extend(n.children)
            if n.type != "struct_specifier":
                continue
            name = n.child_by_field_name("name")
            body = n.child_by_field_name("body")
            if name is None or body is None:
                continue
            fields = []
            for fd in body.children:
                if fd.type != "field_declaration":
                    continue
                t = fd.child_by_field_name("type")
                base = ("scalar",)
                if t is not None and t.type == "struct_specifier":
                    nm = t.child_by_field_name("name")
                    base = ("struct", text(src, nm)) if nm is not None else ("anon",)
                elif t is not None and t.type == "union_specifier":
                    base = ("union",)
                for d in fd.children_by_field_name("declarator"):
                    ty = base
                    while d.type == "array_declarator":
                        ty = ("array", ty)
                        d = d.child_by_field_name("declarator")
                    while d.type in ("pointer_declarator",):
                        ty = ("scalar",)
                        d = d.child_by_field_name("declarator")
                    if d.type == "field_identifier":
                        fields.append((text(src, d), ty))
            out.setdefault(text(src, name), fields)
    return out


def zero(ty):
    return "0" if ty[0] == "scalar" else "{0}"


def is_list(n):
    return n is not None and n.type == "initializer_list"


def elements(src, lst):
    return [c for c in lst.named_children if c.type != "comment"]


def convert(src, node, ty, structs, depth):
    """The positional text for initializer `node` of type `ty`, or the node's
    own text when nothing under it is designated."""
    if not is_list(node):
        return text(src, node)
    els = elements(src, node)
    designated = any(e.type == "initializer_pair" for e in els)
    ind = "    " * (depth + 1)
    if ty[0] == "array":
        vals, pos = {}, 0
        for e in els:
            if e.type == "initializer_pair":
                des = e.child_by_field_name("designator")
                if des is None or des.type != "subscript_designator":
                    raise Unresolved("a field designator in an array")
                idx = int(text(src, des.named_children[0]), 0)
                vals[idx] = convert(src, e.child_by_field_name("value"),
                                    ty[1], structs, depth + 1)
                pos = idx + 1
            else:
                vals[pos] = convert(src, e, ty[1], structs, depth + 1)
                pos += 1
        if not vals:
            return "{0}"
        items = [vals.get(i, zero(ty[1])) for i in range(max(vals) + 1)]
        one = "{ " + ", ".join(items) + " }"
        if "\n" not in one and len(one) <= 72:
            return one
        # one element a line, or eight scalars a line
        if all("\n" not in x and len(x) <= 8 for x in items):
            rows = [", ".join(items[i:i + 8]) for i in range(0, len(items), 8)]
        else:
            rows = items
        return "{\n" + ",\n".join(ind + r for r in rows) + ",\n" + "    " * depth + "}"
    if ty[0] == "struct":
        fields = structs.get(ty[1])
        if fields is None:
            raise Unresolved("struct %s" % ty[1])
        names = [f for f, _ in fields]
        vals, pos = {}, 0
        for e in els:
            if e.type == "initializer_pair":
                des = e.child_by_field_name("designator")
                if des is None or des.type != "field_designator":
                    raise Unresolved("an index designator in a struct")
                f = text(src, des.named_children[0])
                if f not in names:
                    raise Unresolved("%s has no field %s" % (ty[1], f))
                pos = names.index(f)
                vals[pos] = convert(src, e.child_by_field_name("value"),
                                    fields[pos][1], structs, depth + 1)
                pos += 1
            else:
                if pos >= len(fields):
                    raise Unresolved("too many values for struct %s" % ty[1])
                vals[pos] = convert(src, e, fields[pos][1], structs, depth + 1)
                pos += 1
        if not vals:
            return "{0}"
        last = len(fields) - 1  # every member, so -Wextra has nothing to say
        flat = [vals.get(i, zero(fields[i][1])) for i in range(last + 1)]
        one = "{ " + ", ".join(flat) + " }"
        if depth > 0 and "\n" not in one and len(one) <= 72:
            return one
        if not designated:
            return "{ " + ", ".join(vals.get(i, zero(fields[i][1]))
                                    for i in range(last + 1)) + " }"
        lines = []
        for i in range(last + 1):
            v = vals.get(i, zero(fields[i][1]))
            lines.append("%s%s,%s/* %s */" % (ind, v, " " if "\n" not in v else "\n" + ind,
                                              fields[i][0]))
        return "{\n" + "\n".join(lines) + "\n" + "    " * depth + "}"
    if designated:
        raise Unresolved("a designated initialiser of type %s" % (ty,))
    return text(src, node)


def declared_type(src, decl, structs):
    """The type of the object a top-level declaration defines."""
    t = decl.child_by_field_name("type")
    base = ("scalar",)
    if t is not None and t.type == "struct_specifier":
        nm = t.child_by_field_name("name")
        base = ("struct", text(src, nm))
    return base


def rewrite(path, structs, check):
    src, root = parse(path)
    raw = open(path, "rb").read()
    edits, problems = [], []
    for decl in root.children:
        if decl.type != "declaration":
            continue
        for idecl in decl.children_by_field_name("declarator"):
            if idecl.type != "init_declarator":
                continue
            value = idecl.child_by_field_name("value")
            if not is_list(value):
                continue
            if not any(n.type in ("field_designator", "subscript_designator")
                       for n in _walk(value)):
                continue
            ty = declared_type(src, decl, structs)
            d = idecl.child_by_field_name("declarator")
            while d is not None and d.type == "array_declarator":
                ty = ("array", ty)
                d = d.child_by_field_name("declarator")
            try:
                new = convert(src, value, ty, structs, 0)
            except Unresolved as e:
                problems.append((decl.start_point[0] + 1, str(e)))
                continue
            edits.append((value.start_byte, value.end_byte, new))
    for line, why in problems:
        print("%s:%d: left as it is: %s" % (os.path.relpath(path), line, why))
    if edits and not check:
        for s, e, new in sorted(edits, reverse=True):
            raw = raw[:s] + new.encode("utf-8") + raw[e:]
        open(path, "wb").write(raw)
    print("%s: %d initialisers %s" % (os.path.relpath(path), len(edits),
                                      "would be rewritten" if check else "rewritten"))
    return len(problems)


def _walk(n):
    stack = [n]
    while stack:
        x = stack.pop()
        yield x
        stack.extend(x.children)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("files", nargs="+")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args(argv)
    structs = struct_defs(HEADERS + a.files)
    bad = sum(rewrite(f, structs, a.check) for f in a.files)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
