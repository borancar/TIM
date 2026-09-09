#!/usr/bin/env python3
"""What each routine reserves for its locals, from the binary and from the port.

Borland Turbo C puts a routine's locals on the stack and says how many bytes
they take in the prologue: `push bp / mov bp,sp / sub sp,N`. That N is a fact
in the image, so the port's own reservation can be *checked* rather than
trusted - and it is the number that decides whether a `[bp-k]` slot is inside
the frame at all.

Three things are read off each prologue:

    sub     the bytes `sub sp,N` reserves: the locals, and nothing else.
    pushed  registers pushed *after* that. They sit below the locals, so a
            callee's frame lands on them unless they are reserved too.
    shape   whether the routine builds a frame at all. One that never takes
            the address of a local may not, and then it has no slots to find.

The port states its own in `dg_enter(N)`. Where the two disagree the port is
either reserving too little - and a callee's frame is landing inside live
locals, which is the fault dgroup.h warns about - or too much, which is
harmless and still worth knowing, because the number is supposed to be the
original's.

**A routine with no `dg_enter` and a non-zero `sub sp` is not a fault.** It
means the port expressed that routine's locals as C locals, which is what they
are; the reservation only matters where an address is handed out. Those are
listed separately, because together they say how much of the stack the port
still models as guest memory.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import tim
import disasm

sys.path.insert(0, os.path.join(tim.REPO, "reconstruct", "tests"))
import provenance

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


def prologue(off):
    """(sub, pushed, built) for the routine at this image offset.

    **The frame is looked for, not assumed to be first.** Three shapes in this
    binary put something ahead of it: `sound_module_position` loads AX before
    `push bp`, the address recorded for `draw_compressed_bitmap` is the `ljmp`
    through the overlay table with the body behind it, and a routine may build
    no frame at all. Requiring `push bp` at instruction zero called all three
    frameless, which is wrong about two of them.
    """
    d = disasm.image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = False
    seen = []
    for ins in md.disasm(d[off:off + 48], off):
        seen.append((ins.mnemonic, ins.op_str))
        if len(seen) >= 10:
            break
    start = None
    for j in range(len(seen) - 1):
        if seen[j] == ("push", "bp") and seen[j + 1][0] == "mov" \
                and seen[j + 1][1].replace(" ", "") == "bp,sp":
            start = j
            break
    if start is None:
        return 0, 0, False
    #: **`sub sp` can come after the pushes.** Borland emits both orders, and
    #: reading only the first shape called `sound_module_position` frameless -
    #: it is `push bp / mov bp,sp / push di / push si / sub sp,6 / mov si,sp`,
    #: and the 6 is exactly the block the sound module fills in. The port had
    #: it right and this tool reported it as "neither rule" for as long as the
    #: routine has existed. So take the pushes and the `sub` in either order,
    #: and stop at the first instruction that is neither.
    i, sub, pushed = start + 2, 0, 0
    while i < len(seen):
        mn, ops = seen[i]
        if mn == "sub" and ops.startswith("sp,") and sub == 0:
            sub = int(ops.split(",")[1].strip(), 16)
        elif mn == "push" and ops in ("si", "di", "bx", "cx", "dx", "ax"):
            pushed += 2
        else:
            break
        i += 1
    return sub, pushed, True


def port_frames():
    """Each transcribed routine's address, its `dg_enter`, and its slots."""
    addr, enter, slots = {}, {}, collections.defaultdict(set)
    above = {}
    fn = re.compile(r"^[a-zA-Z_].*\b(\w+)\s*\(")
    for path in sorted(glob.glob(os.path.join(tim.REPO, "reconstruct", "**",
                                              "*.c"), recursive=True)):
        transcribed, _ours, _stubs, _bare, _internal = provenance.check(path)
        for name, at in transcribed:
            try:
                addr[name] = int(at, 16)
            except ValueError:
                pass                      # an overlay address, not an image one
        cur = None
        for line in open(path, encoding="utf-8", errors="replace"):
            m = fn.match(line)
            if m and not line.rstrip().endswith(";"):
                cur = m.group(1)
            elif line.startswith("}"):
                cur = None
            # **The array counts too.** A converted routine no longer calls
            # `dg_enter`, and its `uint8_t frame[N]` is then the only record of
            # the frame's size - so a check that only reads `dg_enter` stops
            # watching a routine at exactly the moment the number stops being
            # stated anywhere else.
            m3 = re.search(r"_Alignas\(2\) uint8_t \w+\[(0x[0-9a-fA-F]+|\d+)\]",
                           line)
            if m3 and cur and cur not in enter:
                enter[cur] = int(m3.group(1), 0)
            m2 = re.search(r"\bdg_enter\((0x[0-9a-fA-F]+|\d+)\)", line)
            # **The first one in the body, and only inside a body.** A routine
            # calls `dg_enter` once, in its prologue; taking the last match
            # instead let a later routine's number land on an earlier name and
            # reported `load_bitmaps` as reserving 4 bytes where it says 0xa2.
            if m2 and cur and cur not in enter:
                enter[cur] = int(m2.group(1), 0)
            m4 = re.search(r"=\s*(?:\(uint16_t\)\()?\s*fp\s*(?:\+\s*"
                           r"(0x[0-9a-fA-F]+|\d+))?\s*\)?\s*;", line)
            if m4 and cur:
                slots[cur].add(int(m4.group(1), 0) if m4.group(1) else 0)
            m5 = re.search(r"=\s*(?:\(int16_t \*\))?&\w+\[(0x[0-9a-fA-F]+|\d+)\]",
                           line)
            if m5 and cur:
                slots[cur].add(int(m5.group(1), 0))
            # **A frame above BP is the caller's argument slots, not locals.**
            # `read_into_huge` and `expand_1bpp_to_4bpp` reserve because
            # `huge_add_to` steps the far pointer the caller passed *by value*
            # - `lea ax,[bp+4]` - so what they reserve stands in for arguments
            # already on the guest stack, and the original's `sub sp` is 0.
            # Without this they read as a mismatch, which is the one thing
            # they are not.
            if cur and re.search(r"/\*[^*]*\[bp\+", line):
                above[cur] = True
    return addr, enter, slots, above


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true",
                    help="every routine, not only the ones that disagree")
    ap.add_argument("--top", type=int, default=40)
    args = ap.parse_args()

    addr, enter, slots, above = port_frames()
    locals_only, with_pushed, other, noenter, noframe = [], [], [], [], []
    args_frame = []
    split = []

    for name, at in sorted(addr.items(), key=lambda kv: kv[1]):
        sub, pushed, built = prologue(at)
        have = enter.get(name)
        if have is None:
            (noenter if (built and sub) else noframe).append((name, at, sub))
            continue
        if not built:
            other.append((name, at, have, None, None))
        elif sub == 0 and above.get(name):
            # The reservation stands in for arguments the caller already
            # pushed - see the note in port_frames.
            args_frame.append((name, at, have, sub, pushed))
        elif have == sub:
            locals_only.append((name, at, have, sub, pushed))
        elif have == sub + pushed:
            with_pushed.append((name, at, have, sub, pushed))
        elif have < sub:
            # **A frame can be split.** `read_far`'s `sub sp,0x10a` is one
            # 0x100-byte buffer plus ten bytes of locals that are already C
            # variables carrying their own `[bp-N]`; the array covers the
            # buffer and nothing has to cover the rest. That is smaller than
            # `sub sp` on purpose, and not the mismatch the line below looks
            # for - so it is counted apart, with the shortfall shown, and only
            # a reader can say whether the shortfall is accounted for.
            split.append((name, at, have, sub, pushed))
        else:
            other.append((name, at, have, sub, pushed))

    print("WHAT EACH ROUTINE RESERVES, from the binary and from the port\n")
    print("  `sub sp,N` is the locals and nothing else, and N is the size a")
    print("  `uint8_t frame[N]` would have to be. Registers pushed after it sit")
    print("  *below* the locals and are not locals.\n")
    print("  port reserves the locals            %4d routines"
          % len(locals_only))
    print("  port reserves locals + pushed regs  %4d routines"
          % len(with_pushed))
    print("  frame is the caller's argument slots%4d routines"
          % len(args_frame))
    print("  port reserves part, rest are C locals%4d routines"
          % len(split))
    print("  port reserves something else        %4d routines" % len(other))
    print("  original reserves, port does not    %4d routines" % len(noenter))
    print("  neither reserves anything           %4d routines\n"
          % len(noframe))

    print("**The port follows two rules at once.** dgroup.h says to pass the")
    print("whole frame below BP - the locals and the registers pushed after")
    print("them - and %d routines do; %d pass the locals alone. Both work today,"
          % (len(with_pushed), len(locals_only)))
    print("because the bytes the original used to save SI and DI hold nothing")
    print("the port reads, so a callee's frame landing on them costs nothing.")
    print("It matters for the addresses handed out, and it stops mattering")
    print("entirely once a frame is a `uint8_t frame[N]`: a C array's")
    print("neighbours are its own bytes and a callee's locals are nowhere")
    print("near them.\n")

    if args_frame:
        print("THE FRAME IS THE CALLER'S ARGUMENT SLOTS - `huge_add_to` steps a")
        print("far pointer the caller passed by value, so the port reserves")
        print("what the guest stack already holds and `sub sp` is 0:")
        for name, at, have, sub, pushed in args_frame[:args.top]:
            print("  %-28s %#07x  reserves %#x" % (name, at, have))
        print()

    if split:
        print("PART OF THE FRAME, the rest already C locals:")
        for name, at, have, sub, pushed in split[:args.top]:
            print("  %-28s %#07x  array %#x of sub sp,%#x - %#x in C locals"
                  % (name, at, have, sub, sub - have))
        print()

    if other:
        print("NEITHER RULE - worth reading one at a time:")
        for name, at, have, sub, pushed in other[:args.top]:
            if sub is None:
                print("  %-28s %#07x  dg_enter(%#x), no BP frame in the "
                      "original" % (name, at, have))
            else:
                print("  %-28s %#07x  dg_enter(%#x), sub sp,%#x + %d pushed"
                      % (name, at, have, sub, pushed))
        print()
    # **Do the slots tile the frame?** The port names a local by writing
    # `fp + k`, so the set of k it names should cover the bytes `sub sp`
    # reserved. A gap is a local nobody has found; a k at or past N is a slot
    # outside the frame, which is a reading that cannot be right.
    short, over = [], []
    for name, at, have, sub, pushed in locals_only + with_pushed:
        ks = sorted(slots.get(name, ()))
        if not ks:
            continue
        if max(ks) >= sub:
            over.append((name, at, sub, max(ks)))
        gaps = sub - (max(ks) + 2) if max(ks) + 2 < sub else 0
        if gaps:
            short.append((name, at, sub, max(ks), gaps))
    print("WHERE THE NAMED SLOTS SIT IN THE FRAME\n")
    print("  **A slot at or past the frame's end is a fact and it is wrong.**")
    print("  `sub sp,N` is the whole of the locals, so no `fp + k` with k >= N")
    print("  can be one. Every routine below reserves locals *and* the")
    print("  registers pushed after them, and the slot past the end is where")
    print("  the original saved SI or DI - so these are the same eight, seen a")
    print("  second way.")
    print("  %d routines:" % len(over))
    for name, at, sub, top in over[:args.top]:
        print("      %-26s %#07x  frame[%#x], names fp+%#x"
              % (name, at, sub, top))
    print()
    print("  The other direction says much less, and is here so that it is not")
    print("  mistaken for a finding. A routine whose highest named slot is far")
    print("  below the frame's top usually has **one buffer** at `fp + 0` that")
    print("  is most of the frame - `read_level` reserves 0x216 bytes and names")
    print("  `fp + 0`, which is a 532-byte array and not 532 missing locals.")
    print("  Nothing here knows a slot's width, so it cannot tell the two")
    print("  apart, and the number below is only \"how far the highest name")
    print("  reaches\":")
    print("  %d routines name nothing in the top half of their frame" % len(short))
    for name, at, sub, top, gap in sorted(short, key=lambda r: -r[4])[:8]:
        print("      %-26s %#07x  frame[%#x], highest fp+%#x"
              % (name, at, sub, top))
    print()

    if args.all:
        print("EVERY ROUTINE THAT RESERVES, with the size a frame array needs:")
        for name, at, have, sub, pushed in sorted(locals_only + with_pushed,
                                                  key=lambda r: -r[3]):
            print("  %-28s %#07x  frame[%#x]%s"
                  % (name, at, sub, "  +%d pushed" % pushed if pushed else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
