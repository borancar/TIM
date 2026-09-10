#!/usr/bin/env python3
"""Find what the DGROUP structs have not swallowed yet, by parsing the C.

Two rules, both over a real parse tree rather than a regex, because both are
about *shape* and a regex cannot see shape. `DG16(0x3894)` and
`DG16((uint16_t)(part + 0x0c))` are the same three characters and completely
different problems; so are a constant argument that is a number and a constant
argument that is an address.

    raw          every remaining DG8/DG16/DG32/DGU16 accessor, grouped by
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

And a third of the computed accesses are **not DGROUP data at all**. In the
large model a routine that hands out the address of a local hands out an
ordinary DGROUP offset, so `dg_alloca` reserves a frame and the port writes
`DGU16(v02)` where the original wrote `[bp-2]`. Those have no record to become
- they are one function's stack - and counting them among the work makes the
work look half again as big as it is. `raw` separates them by where the base
was assigned from: a base that came from `fp` is a frame slot, anything else is
a record reached through a pointer.

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

DG = ("DG8", "DG16", "DG32", "DGU16")


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


FRAME_RHS = re.compile(r"^\(uint16_t\)\(\s*fp\b|^fp\b")
DECL = re.compile(r"\b(?:uint16_t|dg_off_t)\s+(\w+)\s*=\s*(.+?);")
ASSIGN = re.compile(r"^\s*(\w+)\s*=\s*(.+?);")
FUNC = re.compile(r"^[a-zA-Z_].*\b(\w+)\s*\(")


def frame_bases(path):
    """(function, variable) pairs whose value came out of `dg_alloca`'s frame.

    Read off the assignment rather than the name, because the names are the
    original's slot numbers - `v02` is `[bp-2]` - and the same name is a
    different slot in every routine that has one.
    """
    out = set()
    cur = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = FUNC.match(line)
        if m and not line.rstrip().endswith(";"):
            cur = m.group(1)
        for pat in (DECL, ASSIGN):
            mm = pat.search(line)
            if mm and FRAME_RHS.match(mm.group(2).strip()):
                out.add((cur, mm.group(1)))
    return out


def rule_raw(paths):
    """Every DG accessor left, split by constant offset versus computed."""
    const = collections.Counter()
    computed = collections.Counter()
    frame = collections.Counter()
    where = collections.defaultdict(list)

    for path in paths:
        src, root = parse(path)
        slots = frame_bases(path)
        holder = {}
        for n in walk(root):
            if n.type == "function_definition":
                d = n.child_by_field_name("declarator")
                if d is not None:
                    name = text(src, d).split("(")[0].strip().lstrip("* ")
                    for k in range(n.start_point[0], n.end_point[0] + 1):
                        holder[k] = name
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
                base = record_base(src, arg)
                if (holder.get(n.start_point[0]), base) in slots:
                    frame[base] += 1
                else:
                    computed[base] += 1
    return const, computed, frame, where


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


def rule_const_addr(paths):
    """A four-digit constant **assigned to a variable that is then an address**.

    This is `mov si, 0x53ab` seen from the C side. In the original a four-digit
    immediate loaded into SI or DI is not a number - those are the registers
    the string and block instructions index through - and the transcription
    carries the same shape:

        uint16_t si = 0x55c3;
        ...
        if (DGU16(si + 0xe) == 0)

    The constant is a DGROUP address with no name, and the rule is the pair:
    a variable initialised or assigned a literal of 0x1000 or more, *and* used
    somewhere as an address - through a `DG*` accessor, through `dg_ptr`, or
    handed to something that takes a pointer. Either half alone is noise: a
    bare four-digit literal is often a mask or a count, and a variable used as
    an address is usually a real pointer already.

    Off the tree rather than a regex because the second half is a *use* of the
    variable elsewhere in the same function, which is shape and not text.

    Not a defect - a worklist, like `raw` and `offset-arg`. The output is what
    `dgrules --rule raw` cannot see, because there is no accessor with the
    constant in it to find.
    """
    out = []
    for path in paths:
        src, root = parse(path)
        for fn in walk(root):
            if fn.type != "function_definition":
                continue
            body = fn.child_by_field_name("body")
            if body is None:
                continue
            # **The enclosing definition is not always the right name.**
            # `copy_protect_screen` has an `#ifndef TIM_COPY_PROTECTION`
            # inside its body, and the C grammar has no preprocessor, so
            # tree-sitter's brace matching runs past the closing brace and
            # swallows the functions after it - which reported a site at
            # game.c:1383 as `copy_protect_screen` when line 1383 is inside
            # `draw_wrapped_text`, 660 lines later. The site's *line* is right
            # either way, so the name comes from the last definition that
            # starts at or before it.
            name = None
            d = fn.child_by_field_name("declarator")
            while d is not None and d.type != "identifier":
                d = d.child_by_field_name("declarator")
            if d is not None:
                name = text(src, d)

            # half one: a variable given a four-digit literal
            lit = {}
            for n in walk(body):
                if n.type == "init_declarator":
                    var = n.child_by_field_name("declarator")
                    val = n.child_by_field_name("value")
                elif n.type == "assignment_expression":
                    # **Plain `=` only.** `heapwalk` writes `si &= 0xfffe` -
                    # a mask, not an address - and an operator-blind rule
                    # reported it beside the real ones, which is the noise
                    # that makes a worklist ignorable.
                    if text(src, n.children[1]).strip() != "=":
                        continue
                    var = n.child_by_field_name("left")
                    val = n.child_by_field_name("right")
                else:
                    continue
                if var is None or val is None or var.type != "identifier":
                    continue
                if val.type != "number_literal":
                    continue
                v = hex_of(text(src, val))
                if v is not None and 0x1000 <= v <= 0xffff:
                    lit.setdefault(text(src, var),
                                   (v, n.start_point[0] + 1))

            if not lit:
                continue

            # half two: is that variable used as an address anywhere?
            used = set()
            for n in walk(body):
                if n.type != "identifier" or text(src, n) not in lit:
                    continue
                p_ = n.parent
                while p_ is not None and p_.type in ("parenthesized_expression",
                                                     "cast_expression",
                                                     "binary_expression"):
                    p_ = p_.parent
                if p_ is None or p_.type != "argument_list":
                    continue
                call = p_.parent
                if call is None or call.type != "call_expression":
                    continue
                callee = text(src, call.child_by_field_name("function"))
                if re.match(r"DG(8|16|32|U16)$", callee) \
                        or callee in ("dg_ptr", "dg_off"):
                    used.add(text(src, n))
            for v in sorted(used):
                val, line = lit[v]
                out.append((os.path.basename(path), line, name, v, val))

    # and the correction the note above describes
    starts = collections.defaultdict(list)
    for path in paths:
        src, root = parse(path)
        for fn in walk(root):
            if fn.type != "function_definition":
                continue
            d = fn.child_by_field_name("declarator")
            while d is not None and d.type != "identifier":
                d = d.child_by_field_name("declarator")
            if d is not None:
                starts[os.path.basename(path)].append(
                    (fn.start_point[0] + 1, text(src, d)))
    for f in starts:
        starts[f].sort()
    fixed = []
    seen_site = set()
    for f, line, name, v, val in out:
        # A definition that swallows the next one - see the note above -
        # reports the same site twice, once from each.
        if (f, line, v, val) in seen_site:
            continue
        seen_site.add((f, line, v, val))
        best = name
        for at, nm in starts.get(f, []):
            if at <= line:
                best = nm
            else:
                break
        fixed.append((f, line, best, v, val))
    return fixed


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rule",
                    choices=("raw", "offset-arg", "truncated", "const-addr",
                             "both"),
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
        const, computed, frame, where = rule_raw(paths)
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
        print("DG accessors into a dg_alloca STACK FRAME - not DGROUP data and")
        print("not a record; this is one routine's locals, and the `[bp-N]`")
        print("comment beside each already says which:")
        print("   %d sites over %d bases" % (sum(frame.values()), len(frame)))
        for base, n in frame.most_common(min(args.top, 8)):
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

    if args.rule in ("const-addr", "both"):
        rows = rule_const_addr(paths)
        print("\nA FOUR-DIGIT CONSTANT used as an ADDRESS - `mov si, 0x53ab`")
        print("seen from the C side, and what `raw` cannot find because there")
        print("is no accessor carrying the constant to group on:")
        print("   %d sites" % len(rows))
        for f, line, fn, var, val in sorted(
                rows, key=lambda r: (-r[4], r[0]))[:args.top]:
            print("   %-16s %5d  %-28s %s = %#06x"
                  % (f, line, fn or "?", var, val))
    return 0


if __name__ == "__main__":
    sys.exit(main())
