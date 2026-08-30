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
    uv run python tools/parts.py --diff out/parts-orig.txt out/parts-port.txt

`--diff` is not `diff`. Every record address differs between the two, so a
plain `diff` reports every line and says nothing. It rewrites each address as
its **index in the walk**, or `-` for a null and `?` for a pointer to something
not on either chain, and compares that. Normalising a pointer to a single
placeholder instead is a trap this file fell into once: it makes "points at
something" and "points at nothing" compare equal, which is the entire content
of the field, and it reported two lists as identical while the one difference
that mattered - a `+0x62` set on one side and zero on the other - was inside
the placeholder.

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
            "f6 %04x f8 %04x a %04x near %5d,%5d "
            "dir %5d vel %5d,%5d wt %5d mom %04x%04x spin %5d "
            "x62 %04x x66 %04x x78 %04x x84 %04x"
            % (name, si, u16(uc, r + 0x04), u16(uc, r + 0x0C),
               s16(uc, r + 0x1E), s16(uc, r + 0x20),
               s16(uc, r + 0x44), s16(uc, r + 0x46),
               u16(uc, r + 0x06), u16(uc, r + 0x08), u16(uc, r + 0x0A),
               s16(uc, r + 0x7A), s16(uc, r + 0x7C),
               s16(uc, r + 0x12),
               s16(uc, r + 0x36), s16(uc, r + 0x38), s16(uc, r + 0x3A),
               u16(uc, r + 0x3E), u16(uc, r + 0x3C), s16(uc, r + 0x9C),
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


PTR_FIELDS = ("x62", "x66", "x78", "x84")


def normalise(lines):
    """Each line with its addresses replaced by where they are in the walk."""
    index = {}
    for n, line in enumerate(lines):
        f = line.split()
        if f and f[0] in ("part", "move"):
            index.setdefault(f[1], "%s#%d" % (f[0], n))

    out = []
    for line in lines:
        f = line.split()
        if not f or f[0] not in ("part", "move"):
            out.append(line)
            continue
        f[1] = index.get(f[1], "?")
        for i, tok in enumerate(f):
            if tok in PTR_FIELDS and i + 1 < len(f):
                # A null and a pointer are *not* the same thing, and collapsing
                # them is what hid the one difference this tool was written for.
                f[i + 1] = ("-" if f[i + 1] == "0000"
                            else index.get(f[i + 1], "?"))
        out.append(" ".join(f))
    return out


def diff_lists(a_path, b_path):
    a = normalise(open(a_path).read().splitlines())
    b = normalise(open(b_path).read().splitlines())
    bad = 0

    for n in range(max(len(a), len(b))):
        x = a[n] if n < len(a) else "(no line)"
        y = b[n] if n < len(b) else "(no line)"
        if x != y:
            bad += 1
            print("  %-4d original %s" % (n, x))
            print("       port     %s" % y)

    print("%d of %d lines differ" % (bad, max(len(a), len(b))))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--insns", type=int, default=900_000_000)
    ap.add_argument("--flip", type=int)
    ap.add_argument("--out")
    ap.add_argument("--diff", nargs=2, metavar=("ORIGINAL", "PORT"),
                    help="compare two dumps, addresses rewritten as indices")
    args = ap.parse_args()

    if args.diff:
        raise SystemExit(1 if diff_lists(*args.diff) else 0)

    if args.flip is None or not args.out:
        raise SystemExit("--flip and --out, or --diff")

    dump_at_flip(args.insns, args.flip, args.out)


if __name__ == "__main__":
    main()
