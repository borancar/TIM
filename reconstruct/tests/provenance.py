"""Every function must say where it came from.

A file whose functions are in address order and each labelled with the image
offset it was read from can be read next to the disassembly; one without it
cannot be checked at all. The convention decays on its own because each
individual omission is trivial and the loss is only felt later, when a verifier
disagrees and there is no way to find the routine it disagreed about. So it is
tested rather than merely written down.

Two things about this check are deliberate:

- **Only the comment block *immediately* above a definition counts.** A version
  that searched a window of preceding lines reports the address of whatever
  routine came before, which makes a file look annotated when it is not - worse
  than no check at all.
- **Three outcomes, not two.** *Transcribed* (an address), *ours* (said so
  explicitly), and *neither*. Only the third is a failure. Collapsing "ours"
  into "not transcribed" loses the distinction the convention exists to record.

This file is the port's own tooling; it is not a transcription.
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
    "tools"))
import cparse
from cparse import parse, text

ADDRESS = re.compile(r"0x[0-9a-fA-F]{4,5}\b")
OURS = re.compile(r"(\bNOT a transcription|\bnot a transcription|"
                  r"\bthe port's own|\bours, not|\bboundary the port chose|"
                  r"^OURS:|\bOURS:|\bThis is ours\b)")
# A routine whose address is known and whose body is not written yet. It must
# not be counted as transcribed - that is the difference between "we know
# where this is" and "we have read it".
STUB = re.compile(r"NOT TRANSCRIBED YET")
BRACE_COMMENT = re.compile(r"^\}\s*/\*")


def definition_name(src, node):
    """The name a function definition declares."""
    d = node.child_by_field_name("declarator")
    while d is not None and d.type != "identifier":
        nxt = d.child_by_field_name("declarator")
        if nxt is None:
            break
        d = nxt
    return text(src, d) if d is not None and d.type == "identifier" else None


def comment_above(node):
    """The comment block directly above this definition, and nothing else.

    **Over the parse tree**, where a comment is a node and "directly above" is
    the previous sibling - not a walk back through lines looking for `*/`,
    which is how the first version of this check came to attribute one
    routine's address to the next.
    """
    prev = node.prev_sibling
    return prev if prev is not None and prev.type == "comment" else None


def first_content_line(block):
    """The first line of a comment block with anything on it."""
    for line in block.split("\n"):
        t = line.strip().lstrip("/*").strip().lstrip("*").strip()
        if t:
            return t
    return ""


def is_static(src, node):
    """**A transcribed routine must not be `static`.**

    Nothing in a binary records C linkage, so `static` on a transcription
    carries no fact from the original - it is a habit, applied because the
    routine happened to have callers in only one translation unit. What it does
    carry is a cost: the symbol is absent from `libtim.so`, so `tools/verify.py`
    has nothing to call and the routine cannot be differentially verified at
    all. Eight polygon routines and `scan_entry_list` sat on the briefing path
    that way. An `ours` helper may be static; a transcription may not.

    A storage class is a node of the definition, which is why this no longer
    asks whether the line starts with the word.
    """
    for ch in node.children:
        if ch.type == "storage_class_specifier" and text(src, ch) == "static":
            return True
        if ch.type in ("function_declarator", "compound_statement"):
            break
    return False


def check(path):
    src, root = parse(path)

    # A comment block must begin its own line.
    #
    # `}/*` - a closing brace and the next routine's provenance comment on one
    # line - reads as neither, and the address above it is then attributed to
    # the routine *after* it: `fill_rect` was recorded at 0x1eb6a, which is
    # `set_palette_pointer`, and the real 0x20079 had no symbol at all. The
    # counts stayed right so `make test` passed, and the wrong name went into
    # every backtrace until one of them contradicted tim.h. A layout rule, so
    # it is read off the text rather than the tree.
    for n, line in enumerate(open(path).read().split("\n"), 1):
        if BRACE_COMMENT.match(line):
            raise SystemExit(
                "@@P@@:%d: `}` and a comment on one line - the comment must "
                "start its own line, or the address above it is attributed to "
                "the routine below it" % (path, n))

    # **The parse has to be a parse.** `cparse` expands the macros the C
    # grammar cannot take; a region it still could not read would hide whole
    # definitions from this count, which is the one thing this check must not
    # do quietly - so what is left is printed beside the counts.
    errs = cparse.errors(root)

    transcribed, ours, stubs, bare, internal = [], [], [], [], []
    for node in root.children:
        if node.type != "function_definition":
            continue
        name = definition_name(src, node)
        if name is None:
            continue
        block_node = comment_above(node)
        block = text(src, block_node) if block_node is not None else None
        # The file header sits above the first function; it names the binary,
        # not the routine, so it must not count as that routine's provenance.
        if block and "corresponds to the original" in block:
            block = None
        line = node.start_point[0] + 1
        if block and STUB.search(block):
            m = ADDRESS.search(block)
            stubs.append((name, m.group(0) if m else "?"))
        # **The address has to be the first thing in the block, not merely
        # somewhere in it.** The convention puts it on its own line at the top,
        # and a search of the whole comment promotes any address *mentioned* in
        # prose into provenance - `vga_write16` is ours and says so, but its
        # comment explains that "it is exactly how VGA:0x13b9 first failed",
        # and that reference alone was enough to file it as transcribed. Seven
        # routines were being counted that way and every one of them is ours.
        elif block and ADDRESS.search(first_content_line(block)):
            transcribed.append((name,
                                ADDRESS.search(first_content_line(block)).group(0)))
            if is_static(src, node):
                internal.append((name, line))
        elif block and OURS.search(block):
            ours.append(name)
        else:
            bare.append((name, line))
    return transcribed, ours, stubs, bare, internal, errs


def main(argv):
    if not argv:
        print("usage: provenance.py FILE.c ...")
        return 2
    total_t = total_o = total_s = 0
    failures = []
    for path in argv:
        t, o, st, b, internal, errs = check(path)
        total_t += len(t)
        total_o += len(o)
        total_s += len(st)
        note = ""
        if errs:
            note = ("   (%d unparsed region(s), first at line %d)"
                    % (len(errs), errs[0].start_point[0] + 1))
        print("%-16s transcribed %-3d ours %-3d stub %-3d unmarked %d%s"
              % (path, len(t), len(o), len(st), len(b), note))
        for name, addr in t:
            print("    %-28s %s" % (name, addr))
        for name in o:
            print("    %-28s ours" % name)
        for name, addr in st:
            print("    %-28s %s  STUB, body not transcribed" % (name, addr))
        for name, ln in b:
            failures.append("%s:%d  %s has neither an address nor an "
                            "explicit 'ours'" % (path, ln, name))
        for name, ln in internal:
            failures.append("%s:%d  %s is transcribed and `static`, so it is "
                            "absent from libtim.so and tools/verify.py cannot "
                            "call it" % (path, ln, name))
    print("\ntranscribed %d, ours %d, stubs %d, unmarked %d"
          % (total_t, total_o, total_s, len(failures)))
    for f in failures:
        print("  FAIL " + f)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
