#!/usr/bin/env python3
"""Prove the transcribed kind table is the one the image carries.

`PART_KINDS_IMAGE` in `reconstruct/src/dgroup.c` is the 58 records of 0x3a
bytes at DGROUP 0x0ea6, written out as C, and `load_part_kinds` puts it in
DGROUP with every hook's segment relocated. The port used to get those bytes
from `TIM.img` like the rest of DGROUP, so everything that compares DGROUP
against the original depends on the two being the same.

This loads the program the way the port does - `io_load_program`, which copies
the image and applies the relocation table - keeps what that left at
0x0ea6..0x1bca, clears the range, runs `load_part_kinds`, and compares. The
relocation is part of what is checked: the image's own segment words are
the unrelocated ones, so a table that forgot the load segment, or added it to
a field the relocation table does not name, differs here.

This file is the port's own tooling; it is not a transcription. GPL-2.0.
"""
import ctypes
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import verify  # noqa: E402  the port's library, built before it is loaded
import tim     # noqa: E402  where the recovered image is

START, END, SIZE = 0x0ea6, 0x1bca, 0x3a
FIELDS = [(0x00, "word_00"), (0x02, "weight"), (0x04, "word_04"),
          (0x06, "word_06"), (0x08, "gravity"), (0x0a, "max_speed"),
          (0x0c, "max_w"), (0x0e, "max_h"), (0x10, "min_w"), (0x12, "min_h"),
          (0x14, "bitmaps_ptr"), (0x16, "bitmaps2_ptr"), (0x18, "word_18"),
          (0x1a, "word_1a"), (0x1c, "refile_level"), (0x1e, "point_count"),
          (0x20, "word_20"), (0x22, "hit.off"), (0x24, "hit.seg"),
          (0x26, "step.off"), (0x28, "step.seg"), (0x2a, "setup.off"),
          (0x2c, "setup.seg"), (0x2e, "flip.off"), (0x30, "flip.seg"),
          (0x32, "settle.off"), (0x34, "settle.seg"), (0x36, "drive.off"),
          (0x38, "drive.seg")]


def main():
    lib = verify.load_lib()
    lib.io_load_program.restype = ctypes.c_int32
    if not lib.io_load_program(tim.IMAGE.encode(), tim.UNPACKED_EXE.encode()):
        sys.exit(f"cannot load {tim.IMAGE} and {tim.UNPACKED_EXE}")

    base = ctypes.c_uint32.in_dll(lib, "dgroup_base").value
    mem = (ctypes.c_ubyte * 0x100000).in_dll(lib, "guest_mem")
    at = ctypes.addressof(mem) + base + START
    loaded = ctypes.string_at(at, END - START)

    ctypes.memset(at, 0xa5, END - START)
    lib.load_part_kinds()
    ours = ctypes.string_at(at, END - START)

    if (END - START) % SIZE or (END - START) // SIZE != 58:
        sys.exit("the range is not 58 records of 0x3a bytes")

    bad = 0
    for k in range((END - START) // SIZE):
        for off, name in FIELDS:
            a = struct.unpack_from("<H", loaded, k * SIZE + off)[0]
            b = struct.unpack_from("<H", ours, k * SIZE + off)[0]
            if a != b:
                bad += 1
                if bad <= 10:
                    print(f"DIFFERS kind {k} {name} (DGROUP 0x{START + k * SIZE + off:04x}): "
                          f"loaded 0x{a:04x}, transcribed 0x{b:04x}")
    if bad:
        print(f"FAIL: {bad} fields of the kind table differ from the loaded image")
        return 1
    print(f"the kind table's 58 records match the loaded image, relocation included")
    return 0


if __name__ == "__main__":
    sys.exit(main())
