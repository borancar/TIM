"""Prove the recovered executable against the stub's own output.

An unpacker that is subtly wrong produces an image that disassembles, loads and
even runs for a while, and every bug it introduces looks like a transcription
mistake for as long as it takes to find. So the recovery is not believed
because it produced a plausible file: it is believed because DOS loading the
emitted EXE puts *exactly* the same bytes in memory, at the same entry point
and on the same stack, as the original stub did when it unpacked itself.

This file is the port's own tooling; it is not a transcription.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
from unlzexe import StubRun

from unicorn.x86_const import (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SS,
                               UC_X86_REG_SP)

PSP = 0x0100


def main():
    if not os.path.exists(tim.UNPACKED_EXE):
        raise SystemExit("no %s - run tools/unlzexe.py first" % tim.UNPACKED_EXE)

    # 1. What the original stub actually produces.
    run = StubRun(PSP)
    regs = run.go()
    size = (run.hi - run.base + 15) & ~15
    ref = run.image(size)

    # 2. What DOS gets when it loads the file we emitted.
    tim.game_dir()
    m = tim.DosMachine(tim.UNPACKED_EXE, verbose=False, psp_seg=PSP)
    got = bytes(m.uc.mem_read(m.load_seg * 16, size))

    ok = True
    if got != ref:
        bad = [i for i in range(size) if got[i] != ref[i]]
        print("IMAGE DIFFERS: %d of %d bytes, first at image offset %#x"
              % (len(bad), size, bad[0]))
        ok = False
    else:
        print("image        %d bytes identical" % size)

    pairs = [("CS", regs["cs"] - run.m.load_seg,
              m.uc.reg_read(UC_X86_REG_CS) - m.load_seg),
             ("IP", regs["ip"], m.uc.reg_read(UC_X86_REG_IP)),
             ("SS", regs["ss"] - run.m.load_seg,
              m.uc.reg_read(UC_X86_REG_SS) - m.load_seg),
             ("SP", regs["sp"], m.uc.reg_read(UC_X86_REG_SP))]
    for name, want, have in pairs:
        mark = "ok" if want == have else "DIFFERS"
        if want != have:
            ok = False
        print("%-12s stub %04x  loaded %04x  %s" % (name, want & 0xFFFF,
                                                    have & 0xFFFF, mark))

    print("ROUND TRIP: " + ("PROVEN" if ok else "FAILED"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
