#!/usr/bin/env python3
"""Take the copy-protection crack out of the recovered image and executable.

The copy this project was built from is patched in **one byte**, and
`copy_protect_screen` is where. Its wait loop is entered on the wrong side:
after `[bp-0x12]` is cleared the routine jumps to 0x0eddd, which *sets* it to
1, and 0x0ede2 leaves when it is not zero - so the screen is drawn and the
routine returns without ever polling, and any answer passes.

    e9 66 01   jmp 0x0ede2    the loop's test, where a Borland `while` enters
    e9 61 01   jmp 0x0eddd    `done = 1`, and the loop never runs

0x0ede2 - 0x0ec7c is 0x166 and 0x0ec7c + 0x161 is 0x0eddd; both land on real
instruction boundaries, which is why it disassembles as ordinary code. No
compiler emits a jump into the middle of a loop body to set that loop's own
exit flag.

**This is not part of the recovery and must not be folded into it.**
`tools/unlzexe.py` recovers exactly what the LZEXE stub produced, and
`tools/verify_unpack.py` proves it byte for byte; that proof is about the
*packing* and has to keep passing against an unmodified recovery. So the order
is: recover, verify, then run this. Running it twice is harmless - it checks
what is there before it writes, and says so.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

IMAGE_OFF = 0x0EC7A         # the displacement's low byte
CRACKED = 0x61              # jmp 0x0eddd - `done = 1`
ORIGINAL = 0x66             # jmp 0x0ede2 - the loop's test


def exe_image_base(data):
    """Where the load image starts inside an EXE: the header is e_cparhdr
    paragraphs, and an image offset is that far in."""
    if data[:2] not in (b"MZ", b"ZM"):
        raise SystemExit("not an EXE: no MZ")
    return struct.unpack_from("<H", data, 8)[0] * 16


def patch(path, off, what):
    with open(path, "rb") as f:
        data = bytearray(f.read())
    if off >= len(data):
        raise SystemExit("%s is too short for offset %#x" % (path, off))

    if data[off] == ORIGINAL:
        print("  %-28s already %#04x - nothing to do" % (what, ORIGINAL))
        return False
    if data[off] != CRACKED:
        raise SystemExit(
            "  %s: expected %#04x or %#04x at %#x, found %#04x - refusing.\n"
            "  This is not the build this patch was measured against."
            % (what, CRACKED, ORIGINAL, off, data[off]))

    data[off] = ORIGINAL
    with open(path, "wb") as f:
        f.write(data)
    print("  %-28s %#04x -> %#04x at %#x" % (what, CRACKED, ORIGINAL, off))
    return True


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--image", default=tim.IMAGE,
                    help="the recovered image (default %(default)s)")
    ap.add_argument("--exe", default=tim.UNPACKED_EXE,
                    help="the recovered executable, which is what the "
                         "emulator actually loads (default %(default)s)")
    ap.add_argument("--check", action="store_true",
                    help="report what is there and change nothing")
    args = ap.parse_args()

    with open(args.exe, "rb") as f:
        exe_off = exe_image_base(bytearray(f.read())) + IMAGE_OFF

    if args.check:
        for path, off, what in ((args.image, IMAGE_OFF, "image"),
                                (args.exe, exe_off, "executable")):
            with open(path, "rb") as f:
                f.seek(off)
                b = f.read(1)[0]
            print("  %-28s %#04x at %#x (%s)"
                  % (what, b, off,
                     "cracked" if b == CRACKED else
                     "original" if b == ORIGINAL else "UNKNOWN"))
        return 0

    print("removing the copy-protection crack:")
    patch(args.image, IMAGE_OFF, "image")
    patch(args.exe, exe_off, "executable")
    print("the loop now enters at its test, and the screen waits for an answer.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
