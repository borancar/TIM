"""Resolve the video driver's API: the thunk table and what it points at.

The game never calls the driver directly. Each module carries a small thunk,
`ljmp [DGROUP offset]`, and DGROUP holds a **far function pointer** that the
loader fills in when it loads `VM.OVL`. So the driver's entry points cannot be
read off the binary at all - they have to be read out of the running machine.

That is the handler table this game dispatches through, and finding it is what
turns "the overlay draws somehow" into a list of entry points with names
implied by their call sites.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from disasm import image, DGROUP

from unicorn import UC_HOOK_INSN, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_CS
import unicorn.x86_const as xc


def thunks():
    """Every `ljmp [imm16]` in the image, with the DGROUP offset it reads."""
    d = image()
    out = []
    i = 0
    while True:
        i = d.find(b"\xff\x2e", i)
        if i < 0 or i >= DGROUP:
            break
        out.append((i, d[i + 2] | (d[i + 3] << 8)))
        i += 2
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--to-flip", type=int, default=8)
    args = ap.parse_args()

    tk = thunks()
    m = drive.machine()
    base = m.load_seg * 16
    flips = {"n": 0}
    drv = {"seg": None}

    def on_w(uc, typ, address, size, value, ud):
        if drv["seg"] is None:
            cs = uc.reg_read(UC_X86_REG_CS)
            if not (base <= cs * 16 < base + DGROUP):
                drv["seg"] = cs

    def on_out(uc, port, size, value, ud):
        if port == 0x3D4 and size == 2 and (value & 0xFF) == 0x0C:
            flips["n"] += 1

    m.uc.hook_add(UC_HOOK_MEM_WRITE, on_w, None, 0xA0000, 0xB0000)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, 260_000_000, on_slice=lambda mm, d: flips["n"] > args.to_flip)

    dg = base + DGROUP
    seen = collections.defaultdict(list)
    for site, ptr in tk:
        off = int.from_bytes(m.uc.mem_read(dg + ptr, 2), "little")
        seg = int.from_bytes(m.uc.mem_read(dg + ptr + 2, 2), "little")
        seen[(seg, off)].append((site, ptr))

    print("driver loaded at segment %04x; %d thunks resolve to %d entry points"
          % (drv["seg"] or 0, len(tk), len(seen)))
    print("\nentry point            thunks (image address -> DGROUP pointer)")
    for (seg, off), sites in sorted(seen.items(), key=lambda kv: kv[0][1]):
        where = "VGA:%04x" % off if seg == drv["seg"] else "%04x:%04x" % (seg, off)
        if seg == 0 and off == 0:
            where = "(null)"
        print("  %-14s %s" % (where,
                              "  ".join("%05x->%#06x" % s for s in sites)))


if __name__ == "__main__":
    main()
