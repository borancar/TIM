"""Recover TIM.EXE from its LZEXE 0.91 packing by *running* the stub.

The stub is not reimplemented here. It is executed under the emulator until it
hands control to the program it unpacked, and the machine is read out at that
instant. That follows the project's standing rule - transcribe or run what the
original does, do not write your own version of it - and it also avoids the
usual failure of a hand-written unpacker, which is to be subtly wrong in a way
that looks like a hundred transcription bugs later on.

The relocation table is *measured* rather than decoded. The stub is run twice,
at two different load segments, and the two images are compared: a word that
differs by exactly the segment delta is a word the stub relocated, and that is
the definition of a relocation entry. Every other byte must be identical, which
is checked - if it is not, the recovery is not deterministic and nothing
downstream can be trusted.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SS,
                               UC_X86_REG_SP)


class StubRun:
    """One execution of the packed program, stopped at the unpacked entry."""

    def __init__(self, psp_seg, max_insns=40_000_000):
        tim.game_dir()
        self.m = tim.DosMachine(tim.PACKED_EXE, verbose=False,
                                max_insns=max_insns, psp_seg=psp_seg)
        self.base = self.m.load_seg * 16
        # The stub's own entry, as a linear address. Anything below this that
        # executes is the program the stub unpacked.
        self.stub_entry = self.base + tim.PACKED_STUB_CS * 16 + tim.PACKED_STUB_IP
        self.hi = self.base          # high-water mark of writes into the image
        self.stopped = None
        self.insns = 0
        self.m.uc.hook_add(UC_HOOK_CODE, self._code)
        self.m.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)

    def _write(self, uc, typ, addr, size, value, ud):
        if self.stopped is None and addr >= self.base:
            if addr + size > self.hi:
                self.hi = addr + size

    def _code(self, uc, addr, size, ud):
        self.insns += 1
        if self.stopped is not None:
            return
        # The stop rule: the first instruction that executes below the stub's
        # own entry point. The stub relocates itself upward and runs high, so
        # coming back down is exactly the hand-over.
        if self.base <= addr < self.stub_entry:
            self.stopped = dict(
                insn=self.insns,
                cs=uc.reg_read(UC_X86_REG_CS), ip=uc.reg_read(UC_X86_REG_IP),
                ss=uc.reg_read(UC_X86_REG_SS), sp=uc.reg_read(UC_X86_REG_SP))
            uc.emu_stop()

    def go(self):
        self.m.run()
        if self.stopped is None:
            raise SystemExit("stub never handed control below %05x "
                             "(ran %d instructions)" % (self.stub_entry, self.insns))
        return self.stopped

    def image(self, size):
        return bytes(self.m.uc.mem_read(self.base, size))


def recover(verbose=True):
    a = StubRun(0x0100)
    ra = a.go()
    b = StubRun(0x0500)
    rb = b.go()

    seg_a, seg_b = a.m.load_seg, b.m.load_seg
    delta = seg_b - seg_a

    # Both runs must agree about how much they touched, and about where the
    # program starts. If they do not, the stub is behaving differently at the
    # two segments and the measurement below is meaningless.
    size_a, size_b = a.hi - a.base, b.hi - b.base
    if size_a != size_b:
        raise SystemExit("stub wrote %d bytes at %04x but %d at %04x"
                         % (size_a, seg_a, size_b, seg_b))
    for k in ("ip", "sp"):
        if ra[k] != rb[k]:
            raise SystemExit("%s differs between runs: %04x vs %04x"
                             % (k, ra[k], rb[k]))
    cs_a, cs_b = ra["cs"] - seg_a, rb["cs"] - seg_b
    ss_a, ss_b = ra["ss"] - seg_a, rb["ss"] - seg_b
    if cs_a != cs_b or ss_a != ss_b:
        raise SystemExit("entry CS/SS not consistent between runs")

    size = (size_a + 15) & ~15
    img_a, img_b = a.image(size), b.image(size)

    # A word that moved by exactly the segment delta is a relocation. A byte
    # that moved by anything else is a bug in this measurement.
    relocs, suspect = [], []
    i = 0
    while i < size - 1:
        if img_a[i] != img_b[i] or img_a[i + 1] != img_b[i + 1]:
            wa = img_a[i] | (img_a[i + 1] << 8)
            wb = img_b[i] | (img_b[i + 1] << 8)
            if (wa + delta) & 0xFFFF == wb:
                relocs.append(i)
                i += 2
                continue
            suspect.append(i)
        i += 1
    if suspect:
        raise SystemExit("%d bytes differ between runs but are not relocations, "
                         "first at image offset %#x" % (len(suspect), suspect[0]))

    # Undo the relocation that run A applied, so the image is segment-neutral
    # again and the emitted EXE carries a relocation table instead.
    img = bytearray(img_a)
    for off in relocs:
        w = (img[off] | (img[off + 1] << 8)) - seg_a
        struct.pack_into("<H", img, off, w & 0xFFFF)

    info = dict(size=size, relocs=relocs, cs=cs_a, ip=ra["ip"],
                ss=ss_a, sp=ra["sp"], insns=ra["insn"],
                seg_a=seg_a, seg_b=seg_b)
    if verbose:
        print("stub handed over after %d instructions" % ra["insn"])
        print("image        %d bytes (%#x paragraphs)" % (size, size // 16))
        print("entry        %04x:%04x" % (info["cs"], info["ip"]))
        print("stack        %04x:%04x" % (info["ss"], info["sp"]))
        print("relocations  %d" % len(relocs))
    return bytes(img), info


def build_exe(img, info, packed_path):
    """Wrap the recovered image in an EXE header with a real relocation table."""
    pk = open(packed_path, "rb").read()
    (cblp, cp, crlc, cparhdr, minalloc, maxalloc, ss, sp, csum, ip, cs,
     lfarlc, ovno) = struct.unpack_from("<13H", pk, 2)
    pk_hdr = cparhdr * 16
    pk_size = (cp - 1) * 512 + cblp - pk_hdr if cblp else cp * 512 - pk_hdr

    # Keep the program's total memory demand the same as the packed file's:
    # image plus minalloc is what DOS actually reserves, and a program that
    # asks DOS how much is free must get the same answer either way.
    total = (pk_size + 15) // 16 + minalloc
    new_min = max(0, total - len(img) // 16)

    nrel = len(info["relocs"])
    hdr_bytes = 0x1C + nrel * 4
    hdr_paras = (hdr_bytes + 15) // 16
    hdr_paras = max(hdr_paras, 2)
    hdr = bytearray(hdr_paras * 16)
    hdr[0:2] = b"MZ"
    total_len = hdr_paras * 16 + len(img)
    # The thirteen header words start *after* the signature, at offset 2.
    struct.pack_into("<13H", hdr, 2,
                     total_len % 512, (total_len + 511) // 512, nrel,
                     hdr_paras, new_min, maxalloc,
                     info["ss"], info["sp"], 0, info["ip"], info["cs"],
                     0x1C, 0)
    for i, off in enumerate(info["relocs"]):
        struct.pack_into("<HH", hdr, 0x1C + i * 4, off & 0x0F, off >> 4)
    return bytes(hdr) + img


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--out", default=tim.UNPACKED_EXE)
    ap.add_argument("--image", default=tim.IMAGE,
                    help="where to write the recovered image itself")
    args = ap.parse_args()

    img, info = recover()
    exe = build_exe(img, info, tim.PACKED_EXE)
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    if os.path.exists(args.out):
        os.remove(args.out)
    open(args.out, "wb").write(exe)
    print("wrote %s (%d bytes)" % (args.out, len(exe)))

    # **Both, from the one recovery.** The image is what every address in this
    # project is an offset into, and it is what `disasm.py`, the port and the
    # hybrid runner actually open - but it used to be written by nobody. A
    # fresh clone ran this tool, got the EXE, and then `native` refused to
    # start with "run tools/unlzexe.py first", naming the step that had just
    # been taken. Writing it here costs nothing: `img` is already in hand, and
    # deriving it later by stripping the EXE header is a second place to get
    # the header size wrong.
    os.makedirs(os.path.dirname(args.image), exist_ok=True)
    if os.path.exists(args.image):
        os.remove(args.image)
    open(args.image, "wb").write(img)
    print("wrote %s (%d bytes)" % (args.image, len(img)))


if __name__ == "__main__":
    main()
