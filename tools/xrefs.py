"""Which instructions name a DGROUP word - the question behind every unnamed field.

A field keeps its address for a name when nothing says what it is for, and that
verdict is only as good as the search behind it. Grepping the port's C answers
about the *port*: a field the original reads from code this port has not
transcribed looks unread there, and so does a field nothing reads at all. The
two are different findings and only the image can tell them apart.

So this reads the image. Every instruction recursive descent reaches is
decoded with capstone's operand detail, and a memory operand with no base and
no index register is a **direct** reference - `mov ax, [0x0096]` - whose
displacement is a DGROUP offset. The answer for one offset is the list of
instructions that name it, with the mnemonic, so a read and a store are told
apart by eye.

**An empty answer is a real answer**, and it is the one that justifies keeping
an address as a name: no instruction the entry point can reach names that word.
It is not proof of nothing - a reference computed into a register (`mov si,
0x53ab` and then `[si]`) is not a direct operand and is not found here, and
neither is one in an overlay, which is a separate binary with its own segment.
`dgrules --rule const-addr` is the tool for the first of those.

A `cs:` or `es:` override names that segment's own data and not DGROUP -
`lcall cs:[0x30f6]` is the sound module's call cell, which `SEGMENT_AT` places
in its code segment - so an overridden operand is skipped.

**And a displacement is only a DGROUP offset while DS holds DGROUP.** It does
not at the entry point: the startup's first instructions run with DS still the
PSP, so `mov bp,[2]` and `mov bx,[0x2c]` at image 0x0c and 0x10 are the PSP's
top-of-memory and environment words and not DGROUP 0x0002 and 0x002c. Four
instructions, all before image 0x14, and they are the only ones in the game
that read through a DS that is not DGROUP.

Offsets are DGROUP offsets, as everywhere in this project: `xrefs.py 0x0096`.
A range is `--range 0x3890:0x3900`, which reports each offset in it that any
instruction names, and `--holes` turns that round and lists the ones nothing
does.

This file is the port's own tooling; it is not a transcription.
"""

# SPDX-License-Identifier: GPL-2.0-only

import argparse
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim                                      # noqa: F401  (the emulator door)
from disasm import image, DGROUP
from codemap import ENTRY, kind_hook_seeds, walk

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone.x86 import X86_OP_MEM, X86_REG_DS


def direct_refs():
    """{DGROUP offset: [(image offset, text), ...]} over all reached code."""
    seen, _, _, _ = walk([ENTRY] + kind_hook_seeds())
    d = image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True

    out = collections.defaultdict(list)
    for pc in sorted(seen):
        ins = next(md.disasm(d[pc:pc + 16], pc), None)
        if ins is None:
            continue
        for op in ins.operands:
            if op.type != X86_OP_MEM:
                continue
            m = op.mem
            if m.base != 0 or m.index != 0:
                continue                        # not a direct reference
            if m.segment not in (0, X86_REG_DS):
                continue                        # `cs:[0x30f6]` is its segment's
            disp = m.disp & 0xffff
            out[disp].append((pc, "%s %s" % (ins.mnemonic, ins.op_str)))
    return out


SO = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                  "..", "reconstruct", "libtim.so")


def _gdb(*exprs):
    out = subprocess.run(["gdb", "-batch"]
                         + [a for e in exprs for a in ("-ex", e)] + [SO],
                         capture_output=True, text=True)
    return out.stdout


def audit():
    """Every offset the original names, against the port's field there.

    The port describes DGROUP as placed objects, so a word the original reads
    or writes should land in a *named* field of one. Two things can be wrong
    and both are findings: the offset lands in a `pad_` or `unknown_` run, and
    a field is hiding in it - which is how `_heaplen` at 0x4d32 and the DOS
    startup's block at 0x0074 were found - or it lands in no object at all,
    and DGROUP has a hole the port has not described.

    The layout comes from the built `libtim.so` rather than from the sources,
    because that is where the linker script actually put everything.
    """
    refs = direct_refs()

    base = int(re.search(r"0x[0-9a-f]+",
                         _gdb("print/x (char*)guest_mem")).group(0), 16)
    dgroup = int(re.search(r"0x[0-9a-f]+",
                           _gdb("print/x dgroup_base")).group(0), 16)

    syms = []
    for line in subprocess.run(["nm", "-S", "--defined-only", SO],
                               capture_output=True, text=True).stdout.splitlines():
        p = line.split()
        if len(p) == 4:
            syms.append((int(p[0], 16), int(p[1], 16), p[3]))

    def owner(addr):
        """The smallest placed object containing `addr` - objects nest."""
        best = None
        for a, size, name in syms:
            if a <= addr < a + size and (best is None or size < best[1]):
                best = (a, size, name)
        return best

    layouts = {}

    def field_at(name, off):
        if name not in layouts:
            rows = []
            for m in re.finditer(r"/\*\s*(\d+)\s*\|\s*(\d+)\s*\*/\s+(.+?);",
                                 _gdb("ptype /o " + name)):
                rows.append((int(m.group(1)), int(m.group(2)), m.group(3).strip()))
            layouts[name] = rows
        hit = None
        for o, size, decl in layouts[name]:
            if o <= off < o + size:
                hit = decl
        return hit

    hiding, orphan = [], []
    for off in sorted(refs):
        own = owner(base + dgroup + off)
        if own is None:
            orphan.append(off)
            continue
        a, _, name = own
        decl = field_at(name, base + dgroup + off - a)
        if decl and re.search(r"\b(pad_|unknown_|_pad)", decl):
            hiding.append((off, name, decl, len(refs[off])))

    for off, name, decl, n in hiding:
        print("%04x  in %s's %s, named by %d instruction%s"
              % (off, name, decl, n, "" if n == 1 else "s"))
    for off in orphan:
        print("%04x  in no placed object, named by %d instruction%s"
              % (off, len(refs[off]), "" if len(refs[off]) == 1 else "s"))

    if hiding or orphan:
        print("\n%d of %d referenced offsets have no named field"
              % (len(hiding) + len(orphan), len(refs)))
        return 1

    print("every one of the %d DGROUP words the original names has a named "
          "field in the port" % len(refs))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("offsets", nargs="*", help="DGROUP offsets, 0x1234")
    ap.add_argument("--range", default="", metavar="LO:HI",
                    help="report every offset named in [LO, HI)")
    ap.add_argument("--holes", action="store_true",
                    help="with --range, list the offsets nothing names")
    ap.add_argument("--audit", action="store_true",
                    help="check every referenced offset against the port's "
                         "fields, over the built libtim.so")
    args = ap.parse_args()

    if args.audit:
        sys.exit(audit())

    refs = direct_refs()
    print("direct DGROUP references over the code recursive descent reaches: "
          "%d distinct offsets\n" % len(refs))

    if args.range:
        lo, hi = [int(p, 16) for p in args.range.split(":")]
        if args.holes:
            quiet = [o for o in range(lo, hi) if o not in refs]
            print("%d of %d offsets in %04x..%04x are named by no instruction"
                  % (len(quiet), hi - lo, lo, hi))
            for o in quiet:
                print("  %04x" % o)
        else:
            for o in sorted(o for o in refs if lo <= o < hi):
                print("%04x  %d" % (o, len(refs[o])))
                for pc, text in refs[o][:8]:
                    print("        %05x  %s" % (pc, text))
        return

    for spec in args.offsets:
        o = int(spec, 16)
        here = refs.get(o, [])
        print("DGROUP %04x: %s" % (o, "%d instructions name it" % len(here)
                                   if here else
                                   "**no instruction names it**"))
        for pc, text in here:
            print("    %05x  %s" % (pc, text))
        print()


if __name__ == "__main__":
    main()
