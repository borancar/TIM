#!/usr/bin/env python3
"""The port's C, parsed - with the macros the grammar cannot read expanded
first.

**A tree-sitter parse of this tree was not a parse until this existed.** Four
things in the port's sources are macros the C grammar has no rule for, and each
one makes tree-sitter abandon the construct it is in and read what follows as
loose expressions:

    struct draw_step DG0124 DGROUP_AT(0x0124) = { ... };
    DG_ASSERT_AT(struct part, kind, 0x04);
    _Static_assert(__builtin_offsetof(struct vm_cs, data_seg) == 0x13a, "..");
    int16_t read_into_huge(uint8_t far * dst, uint16_t count)

A placement macro sits between a declarator and its `=`, which no C grammar
accepts; `DG_ASSERT_AT` and `__builtin_offsetof` take a *type* as an argument,
which no expression grammar accepts; and `far` is defined as nothing at all -
"one kind of pointer here" in tim.h - so `uint8_t far * dst` reads as two type
names in a row.

Measured: a plain parse of the port's sources yields **7,188 ERROR nodes**,
6,112 of them in `dgroup.c`, and inside an ERROR subtree the node types are
wrong - `.hotspots_ptr = 0x02c2` comes out as an `assignment_expression` with
no designator to read. A rule walking that tree is reading soup and cannot say
so. With the four expanded there are **six** left in the whole tree: the
`#ifndef` inside `copy_protect_screen` and one in `sdl.c`, which are
preprocessor conditionals in the middle of a function and beyond any macro
expansion.

What is done here is expansion, not text munging standing in for a parse: each
substitution replaces a known, fixed macro with what the compiler replaces it
with, space for space, so every byte offset and line number is the file's own.

This file is the port's own tooling; it is not a transcription.
"""
import re

try:
    from tree_sitter import Language, Parser
    import tree_sitter_c
except ImportError:                                     # pragma: no cover
    raise SystemExit("tree-sitter is not installed: uv sync")

# The placements, and the assertions that pin them. Blanked whole: no rule
# wants to look inside one, and what is inside is a type where an expression
# belongs.
BLANK_RE = re.compile(r"\b(?:DGROUP_AT|DGROUP_BSS|SEGMENT_AT|DG_ASSERT_AT)"
                      r"\s*\([^()]*\)")
# An offsetof has to leave a `0` behind, or the `_Static_assert` around it
# loses its operand and the error comes back one line further on.
OFFSETOF_RE = re.compile(r"\b__builtin_offsetof\s*\([^()]*\)")
# `far` and `huge` are the Borland tags, defined as nothing on the host.
TAG_RE = re.compile(r"\b(?:far|huge)\b(?=\s*\*)")

_PARSER = Parser(Language(tree_sitter_c.language()))


def expand(text):
    """The source with those four expanded, every offset preserved."""
    text = BLANK_RE.sub(lambda m: " " * len(m.group(0)), text)
    text = OFFSETOF_RE.sub(lambda m: "0" + " " * (len(m.group(0)) - 1), text)
    return TAG_RE.sub("   ", text)


def parse(path):
    """A file's bytes - as expanded - and its parse tree."""
    src = expand(open(path, "rb").read().decode("utf-8", "replace")).encode("utf-8")
    return src, _PARSER.parse(src).root_node


def parse_text(text):
    """The same for source already in hand, which is what a test wants."""
    src = expand(text).encode("utf-8")
    return src, _PARSER.parse(src).root_node


def walk(node):
    """Every node under `node`, in document order.

    With an explicit stack, not recursion: a generated initialiser nests deeply
    enough - a thousand levels - to exceed Python's recursion limit.
    """
    stack = [node]
    while stack:
        n = stack.pop()
        yield n
        stack.extend(reversed(n.children))


def text(src, node):
    return src[node.start_byte:node.end_byte].decode("utf-8", "replace")


def placements(path):
    """Every `DGROUP_AT`/`DGROUP_BSS` placement in a file, as (struct, name,
    address).

    The macro is expanded away before the parse - that is the whole point of
    this module - so the address it carried has to be read here, from the text,
    where it is still a fixed macro invocation with a literal in it. A caller
    that wants the object's *fields* parses as usual; this only recovers the
    number the grammar could not hold.
    """
    out = []
    text_ = open(path, "rb").read().decode("utf-8", "replace")
    for m in re.finditer(r"struct\s+(\w+)\s+(\w+)\s*(?:\[[^\]]*\])?\s*"
                         r"(?:DGROUP_AT|DGROUP_BSS)\((0x[0-9a-fA-F]+)\)", text_):
        out.append((m.group(1), m.group(2), int(m.group(3), 0)))
    return out


def errors(root):
    """The ERROR nodes in a tree - what a caller checks before trusting it."""
    return [n for n in walk(root) if n.type == "ERROR"]
