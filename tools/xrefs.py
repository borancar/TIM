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
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim                                      # noqa: F401  (the emulator door)
from disasm import image, DGROUP
from codemap import ENTRY, kind_hook_seeds, walk

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone.x86 import X86_OP_MEM


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
            disp = m.disp & 0xffff
            out[disp].append((pc, "%s %s" % (ins.mnemonic, ins.op_str)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("offsets", nargs="*", help="DGROUP offsets, 0x1234")
    ap.add_argument("--range", default="", metavar="LO:HI",
                    help="report every offset named in [LO, HI)")
    ap.add_argument("--holes", action="store_true",
                    help="with --range, list the offsets nothing names")
    args = ap.parse_args()

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
