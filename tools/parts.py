"""Dump the original's part list at a page flip, to compare with the port's.

A screen that differs in a hundred pixels does not say *which part* is wrong,
and reasoning backwards from the pixels went astray twice on the credits
screen. The records are also **recycled**: the title machine is freed and the
credits machine built over the same addresses, so an address names one part on
one screen and a different part on the other, and any comparison that keys on
an address is worthless. The lists have to be compared *as lists*, in walk
order, on their contents.

So this walks the same two chains `reconstruct/devdump.c` walks - every part at
DGROUP 0x521b and the moving ones at 0x5179, each threaded through its own first
word - and writes the same line for each. The cue is the same one
`tools/capture.py` takes its reference frames on, the write to CRTC 0x0C that
makes a composed frame visible, so a flip number means the same instant on both
sides.

    uv run python tools/parts.py --flip 295 --out out/parts-orig.txt
    TIM_PARTS=295:out/parts-port.txt ./reconstruct/tim
    diff out/parts-orig.txt out/parts-port.txt

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from disasm import DGROUP

PART_LIST = 0x521B
MOVING_LIST = 0x5179


def u16(uc, at):
    return int.from_bytes(uc.mem_read(at, 2), "little")


def s16(uc, at):
    v = u16(uc, at)
    return v - 0x10000 if v & 0x8000 else v


def dump_chain(uc, dg, name, head, out):
    si = u16(uc, dg + head)
    n = 0
    while si and n < 4096:
        r = dg + si
        out.append(
            "%s %04x kind %2u form %2u pos %5d,%5d size %4d,%4d "
            "f6 %04x f8 %04x a %04x x62 %04x x66 %04x x78 %04x x84 %04x"
            % (name, si, u16(uc, r + 0x04), u16(uc, r + 0x0C),
               s16(uc, r + 0x1E), s16(uc, r + 0x20),
               s16(uc, r + 0x44), s16(uc, r + 0x46),
               u16(uc, r + 0x06), u16(uc, r + 0x08), u16(uc, r + 0x0A),
               u16(uc, r + 0x62), u16(uc, r + 0x66), u16(uc, r + 0x78),
               u16(uc, r + 0x84)))
        si = u16(uc, r)
        n += 1


def dump_at_flip(instructions, flip, path, ips=drive.DEFAULT_IPS):
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    m = drive.machine(ips=ips)
    state = {"flips": 0, "done": False}

    def on_out(uc, port, size, value, ud):
        if port != 0x3D4 or size != 2 or (value & 0xFF) != 0x0C:
            return
        n = state["flips"]
        state["flips"] = n + 1
        if n != flip or state["done"]:
            return

        dg = m.load_seg * 16 + DGROUP
        out = ["flip %d origin %d,%d mode %04x"
               % (n, s16(uc, dg + 0x4EA3), s16(uc, dg + 0x4EA1),
                  u16(uc, dg + 0x4E6B))]
        dump_chain(uc, dg, "part", PART_LIST, out)
        dump_chain(uc, dg, "move", MOVING_LIST, out)

        with open(path, "w") as f:
            f.write("\n".join(out) + "\n")
        state["done"] = True
        print("wrote the part list at flip %d to %s  (%d lines)"
              % (n, path, len(out)))

    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)

    def on_slice(mm, done):
        return state["done"]

    drive.drive(m, instructions, on_slice=on_slice)
    if not state["done"]:
        raise SystemExit("flip %d never happened (%d seen)"
                         % (flip, state["flips"]))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--insns", type=int, default=900_000_000)
    ap.add_argument("--flip", type=int, required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    dump_at_flip(args.insns, args.flip, args.out)


if __name__ == "__main__":
    main()
