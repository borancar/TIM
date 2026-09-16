#!/usr/bin/env python3
"""Prove the port's transcribed data is the image's, at the image's addresses.

The port does not load `TIM.img`. What the game needs of it - DGROUP's
initialised data, and the tables the sound module keeps in its own code segment
- is C objects, each placed by the linker at the address its `DGROUP_AT`,
`DGROUP_BSS` or `SEGMENT_AT` names (see dgroup.h and tools/genld.py). Three things
have to hold for that to be the same memory the original started with, and this
checks all three against `libtim.so`, untouched - no routine called, nothing loaded:

1. **Placement.** Every such object's address, less `guest_mem`'s, is the
   linear address its section names. The linker script is generated and the
   linker refuses overlaps, but a script that placed nothing, or a build that
   linked without it, would put the objects wherever the compiler liked and
   still link.
2. **Contents.** Every initialised object holds exactly the bytes the image has
   there *after DOS relocated it* - computed here from the image and the
   unpacked executable's relocation table at the load segment - and every
   `DGROUP_BSS` object is zero. A far pointer transcribed without `LOAD_SEG`,
   or with it where the relocation table has no entry, differs here.
3. **Nothing left behind.** The port no longer loads the image, so a byte the
   game needs and no object holds would simply be zero. The whole of DGROUP's
   initialised data - 0x0000 to 0x4e4e, where Borland's startup starts zeroing
   - and the sound module's two data blocks inside its code segment must equal
   the image as linked. Those are the ranges a run with every other image byte
   wiped was measured to need: the intro, the solutions and the sequencer's
   trace were unchanged without the rest.

This file is the port's own tooling; it is not a transcription. GPL-2.0.
"""
import ctypes
import glob
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import verify  # noqa: E402  the port's library, built before it is loaded
import tim     # noqa: E402  where the recovered image is

REC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "reconstruct")
LOAD_SEG = 0x0110
IMG_DGROUP = 0x2d3c0
DGROUP = (LOAD_SEG << 4) + IMG_DGROUP
INIT_END = 0x4e4e
# The image ranges the game reads: DGROUP's initialised data, and the sound
# module's tables in segment 2619 - 0x0008..0x020d and 0x30f6..0x30fc.
NEEDED = [(DGROUP, DGROUP + INIT_END),
          ((LOAD_SEG << 4) + 0x26190 + 0x0008, (LOAD_SEG << 4) + 0x26190 + 0x020d),
          ((LOAD_SEG << 4) + 0x26190 + 0x30f6, (LOAD_SEG << 4) + 0x26190 + 0x30fc)]

DG = re.compile(r"^\.guest\.dgroup\.0x([0-9a-fA-F]{4})$")
BSS = re.compile(r"^\.bss\.guest\.dgroup\.0x([0-9a-fA-F]{4})$")
SEG = re.compile(r"^\.guest\.seg\.0x([0-9a-fA-F]{4})\.0x([0-9a-fA-F]{4})$")


def relocated_image():
    """The image as DOS left it in memory, indexed by linear address."""
    img = bytearray(open(tim.IMAGE, "rb").read())
    exe = open(tim.UNPACKED_EXE, "rb").read()
    nrel, = struct.unpack_from("<H", exe, 6)
    table, = struct.unpack_from("<H", exe, 0x18)
    for i in range(nrel):
        off, seg = struct.unpack_from("<HH", exe, table + 4 * i)
        at = (seg << 4) + off
        v, = struct.unpack_from("<H", img, at)
        struct.pack_into("<H", img, at, (v + LOAD_SEG) & 0xffff)
    return bytes(LOAD_SEG << 4) + bytes(img)


def placed_objects():
    """(name, linear address, size, initialised) for every placed object."""
    objs = glob.glob(os.path.join(REC, "*.o")) + glob.glob(os.path.join(REC, "src", "*.o"))
    if not objs:
        sys.exit("no objects under reconstruct/ - run make first")
    out = []
    for obj in objs:
        text = subprocess.run(["readelf", "-SWs", obj], capture_output=True,
                              text=True, check=True).stdout
        sect = {}
        for m in re.finditer(r"^\s*\[\s*(\d+)\]\s+(\S+)", text, re.M):
            sect[m.group(1)] = m.group(2)
        for line in text.splitlines():
            f = line.split()
            if len(f) < 8 or f[3] != "OBJECT" or f[6] not in sect:
                continue
            name, size = f[7], int(f[2], 0)
            s = sect[f[6]]
            if m := DG.match(s):
                out.append((name, DGROUP + int(m.group(1), 16), size, True))
            elif m := BSS.match(s):
                out.append((name, DGROUP + int(m.group(1), 16), size, False))
            elif m := SEG.match(s):
                at = (LOAD_SEG << 4) + (int(m.group(1), 16) << 4) + int(m.group(2), 16)
                out.append((name, at, size, True))
    return sorted(out, key=lambda o: o[1])


def main():
    lib = verify.load_lib()
    path = lib._name
    syms = {}
    for line in subprocess.run(["nm", path], capture_output=True, text=True,
                               check=True).stdout.splitlines():
        f = line.split()
        if len(f) == 3:
            syms.setdefault(f[2], set()).add(int(f[0], 16))
    base = min(syms["guest_mem"])
    mem = bytes((ctypes.c_ubyte * 0x100000).in_dll(lib, "guest_mem"))
    image = relocated_image()

    bad = 0
    objects = placed_objects()
    for name, at, size, init in objects:
        where = {a - base for a in syms.get(name, ())}
        if at not in where:
            bad += 1
            print(f"MISPLACED {name}: should be at guest_mem + 0x{at:05x}, is at "
                  + (", ".join(f"0x{w:05x}" for w in sorted(where)) or "nowhere"))
            continue
        got = mem[at:at + size]
        want = image[at:at + size] if init else bytes(size)
        if got != want:
            bad += 1
            i = next(i for i in range(size) if got[i] != want[i])
            print(f"DIFFERS {name} at +0x{i:x} (linear 0x{at + i:05x}): "
                  f"transcribed {got[i:i + 8].hex()}, image {want[i:i + 8].hex()}")
    if bad:
        print(f"FAIL: {bad} of {len(objects)} placed objects are not the image's")
        return 1

    missing = 0
    for lo, hi in NEEDED:
        for at in range(lo, hi):
            if mem[at] != image[at]:
                if missing < 10:
                    print(f"NOT TRANSCRIBED linear 0x{at:05x}: the image has "
                          f"{image[at]:02x}, the port {mem[at]:02x}")
                missing += 1
    if missing:
        print(f"FAIL: {missing} bytes the game reads are not in any placed object")
        return 1
    print(f"{len(objects)} placed objects, each at its address and holding the image's bytes, "
          f"and nothing the game reads is left out")
    return 0


if __name__ == "__main__":
    sys.exit(main())
