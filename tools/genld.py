#!/usr/bin/env python3
"""Write the linker script that puts the guest's data where the original had it.

The port keeps the guest's megabyte as one block, `guest_mem`, because the game
holds near and far pointers into it. What the image used to put there - DGROUP's
initialised data, and the few tables the sound module keeps inside its own code
segment - is transcribed as C objects now, and each one says where it goes:

    struct part_kind PART_KINDS[58] DGROUP_AT(0x0ea6) = { ... };

`DGROUP_AT`, `DGROUP_BSS` and `SEGMENT_AT` (dgroup.h) are section attributes and
nothing else. This reads the section names back out of the **compiled objects**
- not out of the sources, so a macro used in a comment or behind an `#if` cannot
put a section in the script that no object has, and an object cannot have one
the script leaves out - and writes a script that lays `guest_mem` out byte for
byte:

- `.guest_mem`, initialised, from the bottom of the megabyte to the end of
  DGROUP's initialised data (0x4e4e - Borland's startup zeroes from there), with
  every `SEGMENT_AT` and `DGROUP_AT` object at its address.
- `.guest_bss`, uninitialised, from there to the top, with every `DGROUP_BSS`
  object at its address.

**The placement is the linker's to enforce.** Each object is preceded by an
assignment to its address, so an object that runs into the next one's is
"cannot move location counter backwards" at link time, not a silent overlap;
`SUBALIGN(1)` stops the compiler's own alignment of a large array moving it off
its address. What this script refuses on its own is what the linker would
accept: an address outside its region, and two objects claiming one address,
which the linker would lay end to end.

This file is the port's own tooling; it is not a transcription. GPL-2.0.
"""
import argparse
import re
import subprocess
import sys

LOAD_LINEAR = 0x0110 << 4           # the load segment, as dgroup.h's LOAD_SEG
DGROUP = LOAD_LINEAR + 0x2d3c0      # dgroup.h's IMG_DGROUP, linear
INIT_END = 0x4e4e                   # dgroup.h's DGROUP_INIT_END
TOP = 0x100000                      # GUEST_MEM_BYTES

DG = re.compile(r"^\.guest\.dgroup\.0x([0-9a-fA-F]{4})$")
BSS = re.compile(r"^\.bss\.guest\.dgroup\.0x([0-9a-fA-F]{4})$")
SEG = re.compile(r"^\.guest\.seg\.0x([0-9a-fA-F]{4})\.0x([0-9a-fA-F]{4})$")


def sections(obj):
    """(name, number of objects in it) for each guest section of one object."""
    out = subprocess.run(["readelf", "-SWs", obj], capture_output=True,
                         text=True, check=True).stdout
    index = {}
    for m in re.finditer(r"^\s*\[\s*(\d+)\]\s+(\S+)\s.*\s(\d+)$", out, re.M):
        if m.group(2).startswith((".guest.", ".bss.guest.")):
            index[m.group(1)] = m.group(2)
            # **Alignment 1, or the code that uses the object assumes one it
            # does not have** - see DGROUP_AT in dgroup.h.
            if m.group(3) != "1":
                sys.exit(f"genld: {m.group(2)} in {obj} is aligned to "
                         f"{m.group(3)}; its definition needs DGROUP_AT's aligned(1)")
    count = dict.fromkeys(index.values(), 0)
    for line in out.splitlines():
        f = line.split()
        # Num: Value Size Type Bind Vis Ndx Name
        if len(f) >= 8 and f[3] == "OBJECT" and f[6] in index:
            count[index[f[6]]] += 1
    return count


def main():
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("objects", nargs="+")
    args = ap.parse_args()

    init, bss, owner = [], [], {}
    bad = []
    for obj in args.objects:
        for name, n in sections(obj).items():
            if name in owner:
                bad.append(f"{name}: in {owner[name]} and in {obj}")
                continue
            owner[name] = obj
            if n != 1:
                bad.append(f"{name} in {obj} holds {n} objects, not one")
            if m := DG.match(name):
                off = int(m.group(1), 16)
                if off >= INIT_END:
                    bad.append(f"{name}: DGROUP_AT is for initialised data, "
                               f"below 0x{INIT_END:04x}; use DGROUP_BSS")
                init.append((DGROUP + off, name))
            elif m := BSS.match(name):
                off = int(m.group(1), 16)
                if off < INIT_END:
                    bad.append(f"{name}: DGROUP_BSS is for what the startup "
                               f"zeroes, from 0x{INIT_END:04x}; use DGROUP_AT")
                bss.append((DGROUP + off, name))
            elif m := SEG.match(name):
                seg, off = int(m.group(1), 16), int(m.group(2), 16)
                at = LOAD_LINEAR + (seg << 4) + off
                if at >= DGROUP:
                    bad.append(f"{name}: a code segment's data is below DGROUP")
                init.append((at, name))
            else:
                bad.append(f"{name} in {obj}: not a guest section name")
    if bad:
        sys.exit("genld: " + "\n       ".join(bad))

    lines = [
        "/* Written by tools/genld.py from the objects' section names. Do not edit:",
        "   the placement is in each object's DGROUP_AT, DGROUP_BSS or SEGMENT_AT. */",
        "SECTIONS",
        "{",
        "  .guest_mem ALIGN(0x1000) : SUBALIGN(1)",
        "  {",
        "    guest_mem = .;",
    ]
    for at, name in sorted(init):
        lines.append(f"    . = guest_mem + 0x{at:05x}; KEEP(*({name}))")
    lines += [
        f"    . = guest_mem + 0x{DGROUP + INIT_END:05x};",
        "  }",
        # An explicit address: left to itself the section takes the largest
        # alignment among its inputs and starts past the end of the first.
        f"  .guest_bss guest_mem + 0x{DGROUP + INIT_END:05x} (NOLOAD) : SUBALIGN(1)",
        "  {",
    ]
    for at, name in sorted(bss):
        lines.append(f"    . = guest_mem + 0x{at:05x}; KEEP(*({name}))")
    lines += [
        f"    . = guest_mem + 0x{TOP:05x};",
        "  }",
        f'  ASSERT(ADDR(.guest_bss) == guest_mem + 0x{DGROUP + INIT_END:05x}, '
        '"the uninitialised half of guest_mem does not follow the initialised half")',
        "}",
        "INSERT AFTER .data;",
        "",
    ]
    text = "\n".join(lines)
    try:
        if open(args.output).read() == text:
            return 0
    except OSError:
        pass
    open(args.output, "w").write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
