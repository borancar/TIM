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
import re
import sys

# A function definition at the top level: a line that starts in column 0, has
# a name and a parenthesised list, and ends in `{` or is followed by one.
# At least one type token, then the name. Written this way rather than with a
# greedy character class: that version matched, but left only the *last
# character* of each name, so the report read "s" and "g" and looked fine.
DEF = re.compile(r"^(?:[A-Za-z_][A-Za-z0-9_]*[\s\*]+)+"
                 r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*$")
ADDRESS = re.compile(r"0x[0-9a-fA-F]{4,5}\b")
OURS = re.compile(r"\b(NOT a transcription|not a transcription|"
                  r"the port's own|ours, not|boundary the port chose)\b")
# A routine whose address is known and whose body is not written yet. It must
# not be counted as transcribed - that is the difference between "we know
# where this is" and "we have read it".
STUB = re.compile(r"NOT TRANSCRIBED YET")


def comment_above(lines, i):
    """The comment block directly above line i, and nothing else."""
    j = i - 1
    while j >= 0 and lines[j].strip() == "":
        j -= 1
    if j < 0 or not lines[j].strip().endswith("*/"):
        return None
    block = []
    while j >= 0:
        block.append(lines[j])
        if lines[j].lstrip().startswith("/*"):
            return "\n".join(reversed(block))
        j -= 1
    return None


def definitions(lines):
    """(name, line index of the first line) for every function definition.

    A signature may run over several lines, so lines are joined until the
    parentheses balance. The first version of this required the whole
    signature on one line and therefore **silently skipped** every function
    written over two - it reported 4 transcribed routines when there were 6,
    and a check that quietly misses functions is worse than none.
    """
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if line[:1].isalpha() and "(" in line:
            joined, j = line, i
            while joined.count("(") > joined.count(")") and j + 1 < len(lines):
                j += 1
                joined += " " + lines[j].strip()
            m = DEF.match(joined.rstrip())
            if m and j + 1 < len(lines) and lines[j + 1].strip() == "{":
                out.append((m.group("name"), i))
                i = j + 1
                continue
        i += 1
    return out


def check(path):
    lines = open(path).read().split("\n")
    transcribed, ours, stubs, bare = [], [], [], []
    for name, i in definitions(lines):
        if name in ("if", "for", "while", "switch", "return", "do"):
            continue
        block = comment_above(lines, i)
        # The file header sits above the first function; it names the binary,
        # not the routine, so it must not count as that routine's provenance.
        if block and "corresponds to the original" in block:
            block = None
        if block and STUB.search(block):
            stubs.append((name, ADDRESS.search(block).group(0)
                          if ADDRESS.search(block) else "?"))
        elif block and ADDRESS.search(block):
            transcribed.append((name, ADDRESS.search(block).group(0)))
        elif block and OURS.search(block):
            ours.append(name)
        else:
            bare.append((name, i + 1))
    return transcribed, ours, stubs, bare


def main(argv):
    if not argv:
        print("usage: provenance.py FILE.c ...")
        return 2
    total_t = total_o = total_s = 0
    failures = []
    for path in argv:
        t, o, st, b = check(path)
        total_t += len(t)
        total_o += len(o)
        total_s += len(st)
        print("%-16s transcribed %-3d ours %-3d stub %-3d unmarked %d"
              % (path, len(t), len(o), len(st), len(b)))
        for name, addr in t:
            print("    %-28s %s" % (name, addr))
        for name in o:
            print("    %-28s ours" % name)
        for name, addr in st:
            print("    %-28s %s  STUB, body not transcribed" % (name, addr))
        for name, ln in b:
            failures.append("%s:%d  %s has neither an address nor an "
                            "explicit 'ours'" % (path, ln, name))
    print("\ntranscribed %d, ours %d, stubs %d, unmarked %d"
          % (total_t, total_o, total_s, len(failures)))
    for f in failures:
        print("  FAIL " + f)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
