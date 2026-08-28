"""Dump a loaded driver as it actually is - decompressed and relocated.

`VM.OVL` in the archive is a container of eight per-adapter drivers, and each
one's payload is compressed - the `VGA:` chunk is 6,853 bytes on disk and
expands to about 10 KB. Rather than reverse the decompressor, the driver is
read out of memory once the game has loaded it, which is the same principle the
LZEXE recovery uses: run what the original runs, then look.

The dump is what gets disassembled, so the addresses in it are the addresses
the game executes - `seg:off` with the segment the loader chose.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from disasm import DGROUP

from unicorn import UC_HOOK_MEM_WRITE, UC_HOOK_INSN
from unicorn.x86_const import UC_X86_REG_CS
import unicorn.x86_const as xc


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default="out/res/VM_VGA.mem")
    ap.add_argument("--size", type=lambda v: int(v, 0), default=0x3000)
    ap.add_argument("--to-flip", type=int, default=8)
    ap.add_argument("--seg", type=lambda v: int(v, 0), default=None,
                    help="dump this segment instead of finding the video "
                         "driver by watching who writes pixels. The sound "
                         "driver never touches A000, so it has to be named; "
                         "its segment is in the sound module's own code "
                         "segment at cs:[0x1e7].")
    args = ap.parse_args()

    m = drive.machine()
    base = m.load_seg * 16
    flips = {"n": 0}
    seg = {"v": None}

    def on_w(uc, typ, address, size, value, ud):
        # Whoever is writing pixels is the driver, by definition.
        if seg["v"] is None:
            cs = uc.reg_read(UC_X86_REG_CS)
            if not (base <= cs * 16 < base + DGROUP):
                seg["v"] = cs

    def on_out(uc, port, size, value, ud):
        if port == 0x3D4 and size == 2 and (value & 0xFF) == 0x0C:
            flips["n"] += 1

    m.uc.hook_add(UC_HOOK_MEM_WRITE, on_w, None, 0xA0000, 0xB0000)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, 260_000_000,
                on_slice=lambda mm, d: flips["n"] > args.to_flip)

    if args.seg is not None:
        s = args.seg
    elif seg["v"] is None:
        raise SystemExit("no driver segment seen - nothing wrote to A000")
    else:
        s = seg["v"]
    blob = bytes(m.uc.mem_read(s * 16, args.size))
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    open(args.out, "wb").write(blob)
    print("driver loaded at segment %04x (linear %05x)" % (s, s * 16))
    print("dumped %d bytes -> %s" % (len(blob), args.out))
    print("DOS blocks: %s"
          % ", ".join("%04x+%x%s" % (b[0], b[1], "*" if b[2] else "")
                      for b in m.arena))


if __name__ == "__main__":
    main()
