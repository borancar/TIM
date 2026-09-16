#!/usr/bin/env python3
"""Refuse a `dg_near` anywhere but a store into a near-pointer field.

`dg_near(dgroup, p)` turns a pointer back into the 16-bit offset the original
kept. That is only ever needed at one boundary: where the value is **filed into
guest memory**, because a DGROUP field holds a word, not a host pointer. Used
anywhere else it is the pointer being turned back into a number too early -
passed to a callee that should take the pointer, returned where the pointer
should be answered, held in a local, or compared, which is the case that hides
best: `dg_near(dgroup, si) != dg_near(dgroup, file)` asks what `si != file`
asks, and `dg_near(dgroup, p) == DG50D3.dragged_part_ptr` is
`p == PART_PTR(DG50D3.dragged_part_ptr)`, the field turned into a pointer
where it is read.

So, over a tree-sitter parse of the port's sources, a `dg_near` call must be the
**whole right-hand side of an assignment to a struct field** - `x.f = ...` or
`x->f = ...`, through any subscripts - and that field must be named `..._ptr`
and declared `dg_near_t`, the type that says "a near pointer" (see dgroup.h).
A cast around the call is refused: a field of the right type needs none.

This file is the port's own tooling; it is not a transcription. GPL-2.0.
"""
import argparse
import collections
import glob
import os
import re
import sys

try:
    from tree_sitter import Language, Parser
    import tree_sitter_c
except ImportError:                                     # pragma: no cover
    raise SystemExit("tree-sitter is not installed: uv sync")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REC = os.path.join(ROOT, "reconstruct")
PARSER = Parser(Language(tree_sitter_c.language()))


def walk(node):
    stack = [node]
    while stack:
        n = stack.pop()
        yield n
        stack.extend(reversed(n.children))


def text(src, node):
    return src[node.start_byte:node.end_byte].decode("utf-8", "replace")


def field_types(paths):
    """member name -> set of declared type spellings, over every struct."""
    types = collections.defaultdict(set)
    for path in paths:
        src = open(path, "rb").read()
        for n in walk(PARSER.parse(src).root_node):
            if n.type != "field_declaration":
                continue
            t = n.child_by_field_name("type")
            if t is None:
                continue
            for d in n.children_by_field_name("declarator"):
                # `dg_near_t name;`, `dg_near_t name[4];` - the name is the
                # innermost field_identifier, and an array of near pointers
                # is still a near-pointer field.
                ids = [c for c in walk(d) if c.type == "field_identifier"]
                if ids:
                    types[text(src, ids[0])].add(text(src, t))
    return types


def assigned_field(src, call):
    """The field name if `call` is the whole right-hand side of a store into
    a struct field, else None and a reason."""
    parent = call.parent
    if parent is None or parent.type != "assignment_expression":
        return None, "not the right-hand side of an assignment"
    if parent.child_by_field_name("right") != call:
        return None, "not the right-hand side of an assignment"
    op = parent.child_by_field_name("operator")
    if op is not None and text(src, op) != "=":
        return None, "a compound assignment"
    left = parent.child_by_field_name("left")
    while left is not None and left.type == "subscript_expression":
        left = left.child_by_field_name("argument")
    if left is None or left.type != "field_expression":
        return None, "stored into something that is not a struct field"
    return text(src, left.child_by_field_name("field")), None


def describe(src, call):
    """Say where a refused call sits, from its nearest telling ancestor."""
    n = call.parent
    while n is not None:
        if n.type == "binary_expression":
            op = text(src, n.child_by_field_name("operator"))
            if op in ("==", "!=", "<", ">", "<=", ">="):
                return "compared"
            return f"arithmetic ({op})"
        if n.type == "cast_expression":
            return "cast"
        if n.type == "argument_list":
            fn = n.parent.child_by_field_name("function")
            return f"passed to {text(src, fn)}"
        if n.type == "return_statement":
            return "returned"
        if n.type in ("init_declarator",):
            return "stored in a local"
        if n.type == "assignment_expression":
            return "stored into something that is not a struct field"
        if n.type in ("expression_statement", "compound_statement"):
            break
        n = n.parent
    return "elsewhere"


def main():
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="print the verdict and the counts only")
    args = ap.parse_args()

    sources = sorted(glob.glob(os.path.join(REC, "src", "*.c"))
                     + glob.glob(os.path.join(REC, "*.c")))
    headers = sorted(glob.glob(os.path.join(REC, "*.h"))
                     + glob.glob(os.path.join(REC, "src", "*.h")))
    if not sources:
        sys.exit("check_dg_near: no sources found under reconstruct/")
    types = field_types(sources + headers)

    bad = []
    for path in sources:
        src = open(path, "rb").read()
        for n in walk(PARSER.parse(src).root_node):
            if n.type != "call_expression":
                continue
            fn = n.child_by_field_name("function")
            if fn is None or text(src, fn) != "dg_near":
                continue
            line = n.start_point[0] + 1
            where = os.path.relpath(path, REC)
            field, why = assigned_field(src, n)
            if field is None:
                bad.append((where, line, describe(src, n), text(src, n)))
                continue
            declared = types.get(field, set())
            if not field.endswith("_ptr"):
                bad.append((where, line, f"stored into `{field}`, not a `_ptr` field",
                            text(src, n.parent)))
            elif declared != {"dg_near_t"}:
                bad.append((where, line,
                            f"stored into `{field}`, declared "
                            + (" / ".join(sorted(declared)) or "nowhere"),
                            text(src, n.parent)))

    if bad:
        kinds = collections.Counter(re.sub(r" `.*", "", b[2]).split(" (")[0]
                                    for b in bad)
        if not args.quiet:
            for where, line, why, what in bad:
                print(f"{where}:{line}: {why}: {' '.join(what.split())[:110]}")
        print("FAIL: %d dg_near uses that are not a store into a dg_near_t _ptr field (%s)"
              % (len(bad), ", ".join(f"{v} {k}" for k, v in kinds.most_common())))
        return 1
    print("every dg_near is a store into a dg_near_t _ptr field")
    return 0


if __name__ == "__main__":
    sys.exit(main())
