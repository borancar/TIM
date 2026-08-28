"""Disassemble the recovered image.

Addresses are **image offsets** throughout, as everywhere else in this project;
a `seg:off` is converted on the way in. The listing annotates what it can prove:
a push of a DGROUP offset that lands on a printable string is shown as that
string, which is what makes a cold read of a routine tractable.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

# Measured, not assumed: the Borland startup at 0000:0016 loads DS with the
# segment whose image offset is this. The compiler's copyright banner sits at
# DGROUP+4, which is the corroboration. See docs/executable.md.
DGROUP = 0x2D3C0

_img = None


def image():
    global _img
    if _img is None:
        path = os.path.join(tim.REPO, "out", "TIM.img")
        if not os.path.exists(path):
            exe = open(tim.UNPACKED_EXE, "rb").read()
            import struct
            hdr = struct.unpack_from("<H", exe, 8)[0] * 16
            _img = exe[hdr:]
        else:
            _img = open(path, "rb").read()
    return _img


def dstring(off, maxlen=72):
    """The DGROUP string at `off`, if that is what is there."""
    d = image()
    a = DGROUP + off
    if not (0 <= a < len(d)):
        return None
    # A real reference points at a string's *first* byte. Without this the
    # annotator happily reports the tail of a neighbouring string for any
    # small constant, which makes a listing look informative and is worse
    # than no annotation at all.
    if a == 0 or d[a - 1] != 0:
        return None
    out = bytearray()
    while a < len(d) and d[a] and len(out) < maxlen:
        c = d[a]
        if c < 0x20 and c not in (9, 10, 13):
            return None
        out.append(c)
        a += 1
    if len(out) < 4 or a >= len(d) or d[a] != 0:
        return None
    return out.decode("latin1")


def disasm(start, count=None, end=None, annotate=True):
    d = image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = False
    stop = end if end is not None else len(d)
    lines = []
    n = 0
    for ins in md.disasm(d[start:stop], start):
        txt = "%-8s %s" % (ins.mnemonic, ins.op_str)
        note = ""
        if annotate:
            for tok in ins.op_str.replace(",", " ").split():
                if tok.startswith("0x"):
                    try:
                        v = int(tok, 16)
                    except ValueError:
                        continue
                    s = dstring(v)
                    if s:
                        note = "  ; %r" % s
                        break
        lines.append("%05x  %-24s %s%s"
                     % (ins.address, ins.bytes.hex(), txt, note))
        n += 1
        if count and n >= count:
            break
    return lines


def parse_addr(s):
    if ":" in s:
        seg, off = s.split(":")
        return int(seg, 16) * 16 + int(off, 16)
    return int(s, 0)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("addr", help="image offset (0x1234) or seg:off (1c25:63d4)")
    ap.add_argument("--file", default="",
                    help="disassemble this raw binary instead of the recovered "
                         "image - used for the video driver, which is dumped "
                         "out of memory by tools/dump_overlay.py")
    ap.add_argument("--org", type=lambda v: int(v, 0), default=0,
                    help="address the --file image starts at")
    ap.add_argument("-n", "--count", type=int, default=40)
    ap.add_argument("-e", "--end", default=None)
    args = ap.parse_args()
    if args.file:
        global _img
        blob = open(args.file, "rb").read()
        if args.org:
            blob = b"\x00" * args.org + blob
        globals()["_img"] = blob
    start = parse_addr(args.addr)
    end = parse_addr(args.end) if args.end else None
    for line in disasm(start, args.count if not end else None, end):
        print(line)


if __name__ == "__main__":
    main()
