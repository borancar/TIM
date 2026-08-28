"""Find the video driver's API: every call from the game into VM.OVL.

Composing a screen from measured geometry loses the call graph. The cheap way
back is the one the method prescribes: find the routine everything funnels
through and record the **return address** at its entry. A far call pushes
CS:IP, so at the callee's first instruction [SP] and [SP+2] name the caller.

One hook turns "the overlay does the drawing somehow" into a list of entry
points with their call sites, which is what makes the drawing path
transcribable rather than guessable.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from disasm import DGROUP, image

from unicorn import UC_HOOK_BLOCK, UC_HOOK_INSN
from unicorn.x86_const import UC_X86_REG_SS, UC_X86_REG_SP, UC_X86_REG_CS
import unicorn.x86_const as xc


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--from-flip", type=int, default=6)
    ap.add_argument("--to-flip", type=int, default=20)
    ap.add_argument("--insns", type=int, default=260_000_000)
    args = ap.parse_args()

    m = drive.machine()
    base = m.load_seg * 16
    top = base + DGROUP
    flips = {"n": 0}
    calls = collections.Counter()
    ovl_base = {"seg": None}
    prev_in_image = {"v": False}

    def on_block(uc, address, size, ud):
        in_image = base <= address < top
        in_ovl = (not in_image) and address < 0xA0000 and address >= 0x30000
        if in_ovl and prev_in_image["v"]:
            if args.from_flip <= flips["n"] <= args.to_flip:
                cs = uc.reg_read(UC_X86_REG_CS)
                ss = uc.reg_read(UC_X86_REG_SS)
                sp = uc.reg_read(UC_X86_REG_SP)
                stk = uc.mem_read(ss * 16 + sp, 4)
                ret_ip = stk[0] | (stk[1] << 8)
                ret_cs = stk[2] | (stk[3] << 8)
                caller = ((ret_cs - m.load_seg) & 0xFFFF) * 16 + ret_ip
                entry = address - cs * 16
                if ovl_base["seg"] is None:
                    ovl_base["seg"] = cs
                # The game does not reach the driver with a direct `lcall
                # imm`: there are none. It calls through a **function pointer
                # table**, which is why the target has to be recognised by
                # what it is rather than by the instruction that reached it.
                # A Borland C function opens `push bp`, and the caller has to
                # be a real image offset - without that second test the
                # overlay calling back into the game and resuming looks like a
                # fresh entry point and the list fills with mid-function
                # blocks that nothing calls.
                if not (0 <= caller < DGROUP):
                    return
                if uc.mem_read(address, 1)[0] != 0x55:
                    return
                calls[(cs, entry, caller)] += 1
        prev_in_image["v"] = in_image

    def on_out(uc, port, size, value, ud):
        if port == 0x3D4 and size == 2 and (value & 0xFF) == 0x0C:
            flips["n"] += 1

    m.uc.hook_add(UC_HOOK_BLOCK, on_block)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, args.insns, on_slice=lambda mm, d: flips["n"] > args.to_flip)

    by_entry = collections.Counter()
    sites = collections.defaultdict(set)
    for (cs, entry, caller), n in calls.items():
        by_entry[(cs, entry)] += n
        sites[(cs, entry)].add(caller)

    print("flips %d..%d: %d distinct driver entry points, %d call sites"
          % (args.from_flip, args.to_flip, len(by_entry),
             sum(len(v) for v in sites.values())))
    print("\ndriver entry points, by how often they are called:")
    for (cs, entry), n in by_entry.most_common(30):
        cs_list = sorted(sites[(cs, entry)])
        shown = " ".join("%05x" % c for c in cs_list[:6])
        more = "" if len(cs_list) <= 6 else " +%d more" % (len(cs_list) - 6)
        print("   %04x:%04x  x%-8d called from %s%s" % (cs, entry, n, shown, more))


if __name__ == "__main__":
    main()
