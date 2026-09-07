#!/usr/bin/env python3
"""Find what the DGROUP structs have not swallowed yet, by parsing the C.

Two rules, both over a real parse tree rather than a regex, because both are
about *shape* and a regex cannot see shape. `DG16(0x3894)` and
`DG16((uint16_t)(part + 0x0c))` are the same three characters and completely
different problems; so are a constant argument that is a number and a constant
argument that is an address.

    raw          every remaining DG8/DGS8/DG16/DG32/DGU16 accessor, grouped by
                 whether its offset is a constant - which a struct field can
                 replace - or computed from a variable, which is a record
                 reached through a pointer and needs its type known first.

    offset-arg   a **pointer-like value passed to a function**, which is the
                 shape that hides a near pointer in plain sight:

                     game_fread((uint16_t)(0x627a + si), 1, 1, di);

                 That argument is an address, and the cast to `uint16_t` is
                 what a near pointer looks like once it has been written as
                 arithmetic. With the struct in place it reads
                 `dg_off(&DG3890.font_table_34[si])`, which says which table.
                 The bare form - a hex constant on its own, `load_bitmaps(0x25e8)`
                 - is reported separately: it is the same kind of value and a
                 much weaker signal, since a constant argument is often just a
                 number.

                 Only *unsigned* casts count. A near pointer is a `uint16_t`,
                 and the same shape with a signed cast is arithmetic on a
                 coordinate rather than on an address.

**Neither rule is a failure.** A computed access is a record field and waits on
the record's type; a bare constant may be a genuine number. The output is a
worklist, so it is sorted by how many sites share an offset - the biggest
cluster is the next struct worth writing.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import re
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

try:
    from tree_sitter import Language, Parser
    import tree_sitter_c
except ImportError:                                     # pragma: no cover
    raise SystemExit("tree-sitter is not installed: uv sync")

DG = ("DG8", "DGS8", "DG16", "DG32", "DGU16")


def parse(path):
    src = open(path, "rb").read()
    parser = Parser(Language(tree_sitter_c.language()))
    return src, parser.parse(src).root_node


def text(src, node):
    return src[node.start_byte:node.end_byte].decode("utf-8", "replace")


def walk(node):
    yield node
    for c in node.children:
        yield from walk(c)


def hex_of(s):
    """The value if this token is an integer literal, else None.

    **Decimal counts too, and missing that hid a real gap.** The survey that
    decided which fields the part record has required `0x`, and the
    transcription writes `part + 4`, `part + 6`, `part + 8` for the kind and
    the two flag words - so three named fields sat inside eight bytes of
    padding, and this tool reported the sites as absent rather than as work.
    """
    s = s.strip().rstrip("uUlL")
    try:
        if s.lower().startswith("0x"):
            return int(s, 16)
        return int(s, 10) if s.isdigit() else None
    except ValueError:
        return None


def record_base(src, node):
    """What a computed offset is reached *through*: `part` in `(part + 0x0c)`.

    Taken off the tree rather than by cutting the text at the `+`. The text is
    `(uint16_t)(part + 0x0c)`, and stripping the cast and the brackets by hand
    gives `uint16_t)(part` - which is what the first version of this printed,
    and it looked like six hundred distinct record types instead of one.
    """
    n = node
    while True:
        if n.type == "cast_expression":
            n = n.child_by_field_name("value")
        elif n.type == "parenthesized_expression":
            inner = [c for c in n.children if c.is_named]
            if not inner:
                break
            n = inner[0]
        else:
            break
        if n is None:
            return "?"
    if n.type == "binary_expression":
        left = n.child_by_field_name("left")
        if left is not None:
            return record_base(src, left)
    return text(src, n).strip()


def rule_raw(paths):
    """Every DG accessor left, split by constant offset versus computed."""
    const = collections.Counter()
    computed = collections.Counter()
    where = collections.defaultdict(list)

    for path in paths:
        src, root = parse(path)
        for n in walk(root):
            if n.type != "call_expression":
                continue
            fn = n.child_by_field_name("function")
            if fn is None or text(src, fn) not in DG:
                continue
            args = n.child_by_field_name("arguments")
            inner = [c for c in args.children if c.is_named]
            if not inner:
                continue
            arg = inner[0]
            v = hex_of(text(src, arg))
            line = n.start_point[0] + 1
            if v is not None:
                const[v] += 1
                where[v].append("%s:%d" % (os.path.basename(path), line))
            else:
                computed[record_base(src, arg)] += 1
    return const, computed, where


def rule_offset_arg(paths):
    """A pointer-like value handed to a function."""
    indexed = []        # (uint16_t)(0x627a + si)
    bare = collections.Counter()

    for path in paths:
        src, root = parse(path)
        for n in walk(root):
            if n.type != "call_expression":
                continue
            fn = n.child_by_field_name("function")
            if fn is None or text(src, fn) in DG:
                continue                       # a DG access is the other rule
            fname = text(src, fn)
            args = n.child_by_field_name("arguments")
            if args is None:
                continue
            for a in [c for c in args.children if c.is_named]:
                line = a.start_point[0] + 1
                # (uint16_t)(0xNNNN + expr) - an address written as arithmetic.
                #
                # **The cast has to be unsigned, and that is the filter that
                # makes this rule worth reading.** A near pointer is a
                # `uint16_t`; a signed cast on the same shape is arithmetic on
                # a coordinate - `draw_bitmap((int16_t)(0x208 + slide_a), ...)`
                # is an x position, and 0x208 is 520 pixels, not an offset.
                # Without this the list was a quarter false positives.
                if a.type == "cast_expression" and "uint16_t" in text(
                        src, a.child_by_field_name("type")):
                    # **The cast's own value, not any descendant of it.**
                    # Walking the whole subtree reported the outer call as well
                    # as the inner one for
                    # `adl_write((uint16_t)(0x40 + SX8((uint16_t)(0x222 + n))))`
                    # - the same line twice, and the outer argument is an OPL
                    # register number rather than an address. The address is
                    # the argument whose *own* top-level expression is
                    # `<constant> + <something>`.
                    b = a.child_by_field_name("value")
                    while b is not None and b.type == "parenthesized_expression":
                        inner = [c for c in b.children if c.is_named]
                        b = inner[0] if inner else None
                    if b is not None and b.type == "binary_expression":
                        L = b.child_by_field_name("left")
                        R = b.child_by_field_name("right")
                        op = b.child_by_field_name("operator")
                        v = hex_of(text(src, L)) if L is not None else None
                        if (op is not None and text(src, op) == "+"
                                and v is not None and v >= 0x100
                                and R is not None):
                            indexed.append((os.path.basename(path), line,
                                            fname, text(src, a)))
                # a bare hex constant: the same kind of value, weaker evidence
                elif a.type == "number_literal":
                    v = hex_of(text(src, a))
                    if v is not None and v >= 0x1000:
                        bare[(fname, v)] += 1
    return indexed, bare


def rule_truncated(paths):
    """A 32-bit value assigned to a field that is not 32 bits wide.

    **This one is a defect and not a worklist**, and it exists because the
    migration to structs introduced exactly it. `DG32(0x52ed) = load_palette(..)`
    became `DG52ED.pal_tim_off = ...`, which kept the offset and threw the
    segment away - three palettes lost their far pointers, the levels still
    solved, and only the intro's frame-by-frame comparison caught it.

    The shape is an `(int32_t)` cast on the right of an assignment whose left is
    a struct field. A field that really is 32 bits wide, or the `.dword` view of
    a union, is fine; anything else is losing half a pointer.
    """
    out = []
    for path in paths:
        src, root = parse(path)
        for n in walk(root):
            if n.type != "assignment_expression":
                continue
            L = n.child_by_field_name("left")
            R = n.child_by_field_name("right")
            if L is None or R is None or L.type != "field_expression":
                continue
            lt = text(src, L)
            if not re.match(r"DG[0-9A-F]{4}\.", lt) or lt.endswith(".dword"):
                continue
            if R.type == "cast_expression":
                ty = text(src, R.child_by_field_name("type"))
                if "32" in ty:
                    out.append((os.path.basename(path), n.start_point[0] + 1,
                                text(src, n)[:90]))
    return out


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rule", choices=("raw", "offset-arg", "truncated", "both"),
                    default="both", help="which rule to run (default both)")
    ap.add_argument("--top", type=int, default=20,
                    help="how many rows of each list to print (default %(default)s)")
    ap.add_argument("--sites", type=str, default="",
                    help="a hex offset: print every site that uses it")
    ap.add_argument("files", nargs="*",
                    help="C files to read (default: the port's own sources)")
    args = ap.parse_args()

    paths = args.files or (
        sorted(glob.glob(os.path.join(tim.REPO, "reconstruct", "src", "*.c")))
        + sorted(glob.glob(os.path.join(tim.REPO, "reconstruct", "*.c"))))

    if args.rule in ("raw", "both"):
        const, computed, where = rule_raw(paths)
        if args.sites:
            v = int(args.sites, 16)
            print("sites using %#06x:" % v)
            for s in where.get(v, []):
                print("   ", s)
            return 0
        print("DG accessors with a CONSTANT offset - a struct field can replace")
        print("these, and the biggest cluster is the next struct to write:")
        print("   %d sites over %d distinct offsets" % (sum(const.values()),
                                                        len(const)))
        for off, n in const.most_common(args.top):
            print("      %#06x  %4d" % (off, n))
        print()
        print("DG accessors with a COMPUTED offset - a record reached through a")
        print("pointer; each needs the record's type known before it can move:")
        print("   %d sites over %d bases" % (sum(computed.values()),
                                             len(computed)))
        for base, n in computed.most_common(args.top):
            print("      %-16s %4d" % (base, n))
        print()

    if args.rule in ("truncated", "both"):
        bad = rule_truncated(paths)
        print("A 32-BIT VALUE STORED INTO A 16-BIT FIELD - this loses the top")
        print("half silently, and a far pointer loses its segment:")
        print("   %d sites" % len(bad))
        for f, line, txt in bad[:args.top]:
            print("      %-18s:%-5d %s" % (f, line, txt))
        print()

    if args.rule in ("offset-arg", "both"):
        indexed, bare = rule_offset_arg(paths)
        print("POINTER-LIKE ARGUMENTS, written as arithmetic - each of these is")
        print("an address, and `dg_off(&STRUCT.field[i])` says which table:")
        print("   %d sites" % len(indexed))
        for f, line, fn, txt in indexed[:args.top]:
            print("      %-18s:%-5d %s(%s ...)" % (f, line, fn, txt))
        print()
        print("bare constant arguments >= 0x1000 - the same kind of value and a")
        print("much weaker signal, since a constant argument is often a number:")
        print("   %d sites" % sum(bare.values()))
        for (fn, v), n in bare.most_common(args.top):
            print("      %-24s %#06x  %4d" % (fn, v, n))
    return 0


if __name__ == "__main__":
    sys.exit(main())
