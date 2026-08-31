"""How much of what the original *runs* does the port account for?

The number that gets quoted for a reconstruction is usually routines
transcribed over routines found, and both halves are soft: recursive descent
cannot follow a jump table, so the denominator is a floor, and a routine
transcribed but never reached says nothing about whether the port works.

This asks the answerable question instead. It runs the **original** to a chosen
flip and records every basic block it enters, which is cheap - a block hook, not
an instruction hook. Then it reports how many of the routines actually executed
the port has an address for.

Two address spaces, and they must not be mixed. The game's own code is an image
offset; the video driver lives in `VM.OVL`, loaded wherever DOS put it, and its
routines are recorded in the port as `VM.OVL VGA:0xNNNN`. A count that compares
overlay addresses against image offsets is comparing nothing.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "reconstruct")

OVL_RE = re.compile(r"VM\.OVL VGA:0x([0-9a-f]{1,4})", re.I)


def port_addresses():
    """Every address the port claims, split by which space it is in.

    **Through `reconstruct/tests/provenance.py`, not a regex of its own.** That
    module already decides what counts as a routine's provenance - only the
    comment block immediately above a definition, an address or one of the
    phrases that mean "ours" - and it is the check `make test` runs. A second
    reading of the same convention in a second file is two things to keep in
    step, and the first attempt at one here found 560 addresses where the check
    counts 661, because it did not know the forms.
    """
    sys.path.insert(0, os.path.join(SRC, "tests"))
    import provenance

    image, overlay = set(), set()
    for name in sorted(os.listdir(SRC)):
        if not name.endswith(".c"):
            continue
        lines = open(os.path.join(SRC, name)).read().split("\n")
        for _, i in provenance.definitions(lines):
            block = provenance.comment_above(lines, i)
            if not block or provenance.STUB.search(block):
                continue
            # **The first address in the block, and only the first.** The
            # convention is that a routine's own address opens its comment;
            # everything after it is prose, and prose here is full of DGROUP
            # offsets - 0x3892, 0x44ef - which match the same shape as a code
            # address and are not one. Taking every match found 1105 addresses
            # where the port has 635 routines.
            ovl = OVL_RE.search(block)
            if ovl:
                overlay.add(int(ovl.group(1), 16))
                continue
            a = provenance.ADDRESS.search(block)
            if a:
                image.add(int(a.group(0), 16))
    return image, overlay


def executed(flip, click, insns):
    """Blocks the original enters, as image offsets and overlay offsets."""
    from unicorn import UC_HOOK_BLOCK, UC_HOOK_INSN
    import unicorn.x86_const as xc

    m = drive.machine()
    base = m.load_seg * 16
    img, ovl = set(), set()
    drv = {"seg": None}
    state = {"n": 0, "stop": False}

    def on_block(uc, address, size, user):
        # The driver's segment is not known until it has been loaded; until
        # then an overlay address would be recorded as a wild image offset, so
        # anything outside the image is simply not recorded.
        off = address - base
        if 0 <= off < 0x2E000:
            img.add(off)
        elif drv["seg"] is not None:
            d = address - drv["seg"] * 16
            if 0 <= d < 0x3000:
                ovl.add(d)

    def on_out(uc, port, size, value, ud):
        if port == 0x3D4 and size == 2 and (value & 0xFF) == 0x0C:
            n = state["n"]
            state["n"] = n + 1
            for at, cx, cy in click:
                if n == at:
                    m.mouse_input(cx, cy, 1)
                elif n == at + 2:
                    m.mouse_input(cx, cy, 0)
            if n >= flip:
                state["stop"] = True
                uc.emu_stop()

    m.uc.hook_add(UC_HOOK_BLOCK, on_block)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)

    from disasm import DGROUP
    dg = base + DGROUP

    def on_slice(mm, done):
        # Where VM.OVL was loaded. DGROUP 0x43b6 is one of the driver vectors
        # the loader fills in - a far pointer - so its segment half says where
        # the overlay is. Nothing in the image writes it, which is why it is
        # read out of the running machine rather than worked out.
        if drv["seg"] is None:
            v = mm.uc.mem_read(dg + 0x43b8, 2)
            seg = v[0] | (v[1] << 8)
            if seg:
                drv["seg"] = seg
        return state["stop"]

    drive.drive(m, insns, on_slice=on_slice)
    return img, ovl, state["n"]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--flip", type=int, default=260)
    ap.add_argument("--click", action="append", default=["200:320:200"],
                    metavar="FLIP:X:Y")
    ap.add_argument("--insns", type=int, default=150_000_000)
    args = ap.parse_args()

    click = [tuple(int(v, 0) for v in c.split(":")) for c in args.click]
    image, overlay = port_addresses()
    print("the port records %d image addresses and %d overlay addresses"
          % (len(image), len(overlay)))

    img, ovl, flips = executed(args.flip, click, args.insns)
    print("the original entered %d image blocks and %d overlay blocks "
          "reaching flip %d" % (len(img), len(ovl), flips))

    # A block is not a routine: a routine is many blocks, and only its first is
    # an address the port would record. So the question asked is the useful one
    # round - of the addresses the port claims, how many were actually run?
    for what, claimed, seen in (("image", image, img), ("overlay", overlay, ovl)):
        hit = claimed & seen
        print("  %-7s: %d of the port's %d addresses were executed (%.0f%%)"
              % (what, len(hit), len(claimed),
                 100.0 * len(hit) / max(1, len(claimed))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
