#!/usr/bin/env python3
"""Refuse a typed handle used as a boolean.

The port's handles - `struct part *`, `struct belt *`, `struct rope *`,
`struct rect_list_entry *`, `struct game_file *`, `struct bitmap *` - are views of DGROUP: `PART_PTR(p)` is
`dgroup + p`, so the original's "no part", an offset of 0, is DGROUP:0 and
**never NULL**. Every `*_NONE` sentinel exists for that reason. It follows
that `if (p)`, `p ? ... : ...`, `!p`, `p && ...` and `... || p` on a handle
are always true where the original tested an offset against 0, and
`p == NULL` is always false - `select_field_2_or_4` read a belt at DGROUP:0
where the original's `or si,si` returned 0 - and the
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
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cparse
from cparse import parse, text, walk

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HANDLES = ("part", "belt", "rope", "rect_list_entry", "game_file", "bitmap")
HEAP = ("heap_calloc_far", "heap_malloc_far")


def declarator_name(src, node):
    """The identifier a declarator eventually names, and whether it is a
    pointer."""
    star = False
    n = node
    while n is not None and n.type != "identifier":
        if n.type == "pointer_declarator":
            star = True
        n = n.child_by_field_name("declarator")
    return (text(src, n) if n is not None else None), star


def handle_vars(src, fn):
    """Every variable in this function declared as a handle - locals and
    parameters both.

    A parameter was the case the regex version missed by construction: it read
    declarations out of the body's text, and `struct part *part` in the
    signature is not in the body.
    """
    out = set()
    for n in walk(fn):
        if n.type not in ("declaration", "parameter_declaration"):
            continue
        t = n.child_by_field_name("type")
        if t is None or t.type != "struct_specifier":
            continue
        name = t.child_by_field_name("name")
        if name is None or text(src, name) not in HANDLES:
            continue
        for d in n.children:
            if d.type in ("init_declarator", "pointer_declarator",
                          "identifier", "array_declarator"):
                target = d.child_by_field_name("declarator") \
                    if d.type == "init_declarator" else d
                v, star = declarator_name(src, target)
                if v and star:
                    out.add(v)
    return out


def bare(src, node, names):
    """The identifier this node is, if it is one of `names` and nothing more.

    `p` is a boolean use; `p->next`, `p[1]` and `f(p)` are not, and the
    difference is the node's type rather than what characters sit beside it.
    """
    while node is not None and node.type == "parenthesized_expression":
        node = next((c for c in node.children if c.is_named), None)
    if node is not None and node.type == "identifier":
        t = text(src, node)
        if t in names:
            return t
    return None


def last_assignment(src, fn, name, before):
    """The right-hand side last given to `name` above line `before`.

    The regex version looked at the three preceding *lines* for an allocation,
    which is a window rather than a rule. The tree gives the assignment itself,
    so the question "was this handle just allocated" is answered by the
    assignment that actually precedes the test.
    """
    best = None
    for n in walk(fn):
        if n.start_point[0] + 1 >= before:
            continue
        if n.type == "init_declarator":
            v, _ = declarator_name(src, n.child_by_field_name("declarator"))
            rhs = n.child_by_field_name("value")
        elif n.type == "assignment_expression":
            v = bare(src, n.child_by_field_name("left"), {name})
            rhs = n.child_by_field_name("right")
        else:
            continue
        if v == name and rhs is not None:
            if best is None or rhs.start_point[0] > best.start_point[0]:
                best = rhs
    return best


def fresh_from_heap(src, fn, name, line):
    """Was this handle's last assignment the near heap's answer?

    `heap_calloc_far` and `heap_malloc_far` answer NULL when they refuse - the
    offset 0 the original tests - so the test straight after an allocation is
    the one place a handle is rightly compared with NULL.
    """
    rhs = last_assignment(src, fn, name, line)
    if rhs is None:
        return False
    for n in walk(rhs):
        if n.type == "call_expression":
            f = n.child_by_field_name("function")
            if f is not None and text(src, f) in HEAP:
                return True
    return False


def scan(path):
    """Every boolean use of a handle in one file, as (line, name, why)."""
    src, root = parse(path)
    hits = []
    for fn in walk(root):
        if fn.type != "function_definition":
            continue
        names = handle_vars(src, fn)
        if not names:
            continue
        for n in walk(fn):
            found = []          # a `p && q` of two handles is two faults
            v = None
            why = "used as a boolean"
            if n.type in ("if_statement", "while_statement", "do_statement"):
                v = bare(src, n.child_by_field_name("condition"), names)
            elif n.type == "conditional_expression":
                v = bare(src, n.child_by_field_name("condition"), names)
            elif n.type == "unary_expression" and \
                    text(src, n.child(0)) == "!":
                v = bare(src, n.child_by_field_name("argument"), names)
            elif n.type == "binary_expression":
                op = text(src, n.child_by_field_name("operator") or n.child(1))
                left = n.child_by_field_name("left")
                right = n.child_by_field_name("right")
                if op in ("&&", "||"):
                    found = [x for x in (bare(src, left, names),
                                         bare(src, right, names)) if x]
                elif op in ("==", "!="):
                    l, r = bare(src, left, names), bare(src, right, names)
                    other = text(src, right) if l else (text(src, left) if r else "")
                    if (l or r) and other.strip() == "NULL":
                        v = l or r
                        line = n.start_point[0] + 1
                        if fresh_from_heap(src, fn, v, line):
                            v = None
                        else:
                            why = "compared with NULL"
            for name in ([v] if v else []) + found:
                hits.append((n.start_point[0] + 1, name,
                             why + ": " + " ".join(text(src, n).split())[:70]))
    return sorted(set(hits))


def main():
    paths = sys.argv[1:] or (
        sorted(glob.glob(os.path.join(REPO, "reconstruct", "src", "*.c")))
        + sorted(glob.glob(os.path.join(REPO, "reconstruct", "*.c"))))
    # **A scan of nothing is not a pass.** Run from anywhere but tools/, the
    # default glob found no sources and this printed its all-clear over zero
    # files - which is how a copy under test "accepted" a tree it never read.
    if not paths:
        print("FAIL: no sources found to scan under %s" % REPO)
        return 1
    bad = 0
    for p in paths:
        src, root = parse(p)
        # **A file the parser could not read is not a file with no hits.**
        # `cparse` expands the macros the C grammar cannot take; anything left
        # is said out loud rather than passed over.
        errs = cparse.errors(root)
        if errs:
            print("note %s: %d unparsed region(s), first at line %d"
                  % (os.path.relpath(p, REPO), len(errs),
                     errs[0].start_point[0] + 1))
        for line, v, why in scan(p):
            print("FAIL %s:%d: handle `%s` %s"
                  % (os.path.relpath(p, REPO), line, v, why))
            bad += 1
    if not bad:
        print("no typed handle is used as a boolean or compared with NULL")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
