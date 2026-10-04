#!/usr/bin/env python3
"""The port's C, parsed - with the macros the grammar cannot read expanded
first.

**A tree-sitter parse of this tree was not a parse until this existed.** Some
things in the port's sources are macros the C grammar has no rule for, and each
one makes tree-sitter abandon the construct it is in and read what follows as
loose expressions:

    _Static_assert(__builtin_offsetof(struct vm_cs, data_seg) == 0x13a, "..");
    int16_t read_into_huge(uint8_t far * dst, uint16_t count)

`__builtin_offsetof` takes a *type* as an argument, which no expression
grammar accepts; and `far` is defined as nothing at all - "one kind of
pointer here" in tim.h - so `uint8_t far * dst` reads as two type names in a
row. (A placement macro between a declarator and its `=` was the third, until
the placements went on 2026-09-29.)

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
import os
import re

try:
    from tree_sitter import Language, Parser
    import tree_sitter_c
except ImportError:                                     # pragma: no cover
    raise SystemExit("tree-sitter is not installed: uv sync")

# An offsetof has to leave a `0` behind, or the `_Static_assert` around it
# loses its operand and the error comes back one line further on.
OFFSETOF_RE = re.compile(r"\b__builtin_offsetof\s*\([^()]*\)")
# `far`, `huge`, `near` and `interrupt` are the Borland tags, defined as nothing on the host, and
# `SDLCALL` is SDL's calling-convention tag in the same position.
TAG_RE = re.compile(r"\b(?:far|huge|near|interrupt)\b")
# **What only Turbo C++ reads**: the body of an `#ifdef __TURBOC__` branch -
# inline `asm`, the pseudo-registers `_CX`/`_DX`, a declaration the original
# made its own way. The tools that parse are about the host's code, so the
# branch is blanked up to its `#else` or `#endif`. Nothing nests inside one.
TCC_BRANCH_RE = re.compile(r"^[ \t]*#[ \t]*ifdef[ \t]+__TURBOC__\b.*?"
                           r"(?=^[ \t]*#[ \t]*(?:else|endif)\b)",
                           re.M | re.S)
# `PACKED` and `NONSTRING` are dgroup.h's spellings of a host attribute, which
# vanish under Turbo C++ 3.0; after a closing brace or a declarator the
# grammar would read either as a second declarator.
ATTR_RE = re.compile(r"\b(?:PACKED|NONSTRING)\b")
SDLCALL_RE = re.compile(r"\bSDLCALL\b")
# **A conditional *inside* a function body.** `copy_protect_screen` has an
# `#ifndef TIM_COPY_PROTECTION` around a label, which the C grammar cannot take
# in an expression position - and the damage is not local: brace matching runs
# past the routine's end and swallows every definition after it, which took
# game.c from 300-odd top-level functions to five. Blanking the *directive*
# lines - not the code between them - leaves both branches in the tree, which
# for counting definitions and finding shapes is right. It would be wrong for a
# tree with an `#if 0` block in it; there is none, and a rule that needed the
# preprocessor's answer rather than the parser's would have to say so.
PPCOND_RE = re.compile(r"^[ \t]*#[ \t]*(?:if|ifdef|ifndef|else|elif|endif)\b"
                       r".*$", re.M)

_PARSER = Parser(Language(tree_sitter_c.language()))


def expand(text):
    """The source with those four expanded, every offset preserved."""
    text = TCC_BRANCH_RE.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    text = OFFSETOF_RE.sub(lambda m: "0" + " " * (len(m.group(0)) - 1), text)
    text = PPCOND_RE.sub(lambda m: " " * len(m.group(0)), text)
    text = SDLCALL_RE.sub("       ", text)
    text = ATTR_RE.sub(lambda m: " " * len(m.group(0)), text)
    return TAG_RE.sub(lambda m: " " * len(m.group(0)), text)


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


def placements(path=None):
    """Every public DGROUP object and its address, as (None, name, address):
    read from `out/v<version>/link/exe/TIM.MAP`, the map of the last `tools/link.py`,
    whose TIM.EXE is the original's byte for byte - so the addresses are the
    image's. `path` is ignored; the map covers every module. Empty when there
    has been no link."""
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from version import OUT
    mp = os.path.join(OUT, "link", "exe", "TIM.MAP")
    if not os.path.exists(mp):
        return []
    out = []
    seen = set()
    for m in re.finditer(r"^ 2D3C:([0-9A-F]{4})(?: idle)?\s+_(\w+)\s*$",
                         open(mp, errors="replace").read(), re.M):
        if m.group(2) not in seen:
            seen.add(m.group(2))
            out.append((None, m.group(2), int(m.group(1), 16)))
    return out


def errors(root):
    """The ERROR nodes in a tree - what a caller checks before trusting it."""
    return [n for n in walk(root) if n.type == "ERROR"]
