#!/usr/bin/env python3
"""Read what `TIM_SLOTS` measured: whose locals another routine reaches.

The port models every `dg_enter` frame slot as a DGROUP offset, and the only
ones that have to be are those another routine is given the address of. Nothing
in the port's own sources says how far a callee reads from such an address - so
`draw_part_extra`'s `v02`, which is only ever `DG16(v02)`, is really `x[2]` of
an array whose base is the slot two along, and unpacking it into a C local left
a polygon reading garbage.

The emulator can answer it, because in the large model SS *is* DGROUP and
Borland leaves a frame chain in memory. `slots.c` records one line per distinct
(owner, reader, local) triple; this names them and says what it means for the
unpacking:

    routine F   [bp-N]   read by G, 1 frame down

Read the **distance** carefully, and note which way it runs. `slots.c` records
`ownerBP - address`, so the number is how far *below* the owner's BP the byte
is - which is exactly how the port already writes a local, `[bp-N]`. A routine
fetching its own arguments reads *above* its BP, which lands below the caller's
locals and so comes out as a distance larger than the caller's frame; and the
port states every frame's size in its own source, as the argument to
`dg_enter`. So the line between "a local of the caller" and "an argument or
something further out" is not a guess here: it is `dg_enter`'s number, read
from the routine the record names.

**It sees only what the emulator still runs.** A routine dispatched to the
port writes `guest_mem` from C and never passes through Unicorn, so nothing it
does appears here. The reaches that motivated this - `draw_polygon` walking
three points from an address `draw_part_extra` handed it - are all in
dispatched code and all invisible. What it does cover is the emulated side,
which is the part the port's own sources cannot be read for at all.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import gensyms
import glob
import re


def frame_sizes():
    """Each routine's frame, as the port's own `dg_enter` states it."""
    fn = re.compile(r"^[a-zA-Z_].*\b(\w+)\s*\(")
    out = {}
    for path in sorted(glob.glob(os.path.join(ROOT, "reconstruct", "src", "*.c"))):
        cur = None
        for line in open(path):
            m = fn.match(line)
            if m and not line.rstrip().endswith(";"):
                cur = m.group(1)
            m2 = re.search(r"dg_enter\((0x[0-9a-fA-F]+|\d+)\)", line)
            if m2 and cur:
                out[cur] = int(m2.group(1), 0)
    return out


def named(syms, addr, base):
    """The routine containing this linear address, and how far into it."""
    off = addr - base
    best = None
    for at in syms:
        if at <= off and (best is None or at > best):
            best = at
    if best is None or off - best > 0x2000:
        return None, off
    return syms[best][0], off - best


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="the file TIM_SLOTS wrote")
    ap.add_argument("--image-base", type=lambda s: int(s, 0), default=0x110 * 16,
                    help="where the program was loaded (default %(default)#x)")
    ap.add_argument("--beyond", action="store_true",
                    help="also list the hits that fall outside the owner's "
                         "own frame - arguments, or a frame this tool could "
                         "not size")
    ap.add_argument("--top", type=int, default=30)
    args = ap.parse_args()

    syms = gensyms.collect()
    sizes = frame_sizes()
    reach = collections.defaultdict(set)      # owner -> {(local, reader, depth)}
    beyond = collections.defaultdict(set)
    unnamed = 0

    for line in open(args.log):
        if line.startswith("#") or not line.strip():
            continue
        o, r, local, depth, kind = line.split()
        owner, _ = named(syms, int(o, 16), args.image_base)
        reader, _ = named(syms, int(r, 16), args.image_base)
        local, depth = int(local), int(depth)
        if owner is None:
            unnamed += 1
            continue
        where = reach if local <= sizes.get(owner, 0) else beyond
        where[owner].add((local, reader or "?", depth, kind))

    def show(title, table, note):
        print(title)
        print(note + "\n")
        rows = sorted(table.items(), key=lambda kv: -len(kv[1]))
        for owner, hits in rows[:args.top]:
            offs = sorted({h[0] for h in hits})
            who = sorted({h[1] for h in hits})
            print("  %-28s frame %#-6x %d: %s"
                  % (owner, sizes.get(owner, 0), len(offs),
                     " ".join("[bp-%#x]" % o for o in offs[:10])))
            print("  %-28s   by %s" % ("", ", ".join(who[:5])))
        print("  %d routines, %d triples\n"
              % (len(table), sum(len(v) for v in table.values())))

    show("ROUTINES WHOSE OWN LOCALS ANOTHER ROUTINE TOUCHES",
         reach,
         "  the distance is within the frame `dg_enter` reserves, so these are\n"
         "  that routine's `[bp-N]` locals and the slot cannot become a plain\n"
         "  C local while the reader exists.")
    if args.beyond:
        show("HITS OUTSIDE THE OWNER'S OWN FRAME", beyond,
             "  further below the owner's BP than its frame reaches: an\n"
             "  argument fetch, or a routine whose frame this could not size.")
    else:
        print("%d routines have hits outside their own frame (--beyond)"
              % len(beyond))
    if unnamed:
        print("%d records name no transcribed routine as the owner" % unnamed)
    return 0


if __name__ == "__main__":
    sys.exit(main())
