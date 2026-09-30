"""Recover TIM.EXE 1.11 (The Even More Incredible Machine) from its RNC packing.

1.11 is packed twice with RNC ProPack, method 1, both layers keyed. Running
the stub under the emulator - what tools/unlzexe.py does for 1.00's LZEXE -
does not work here: both RNC stubs come out of the emulator with output that
differs from run to run and from load address to load address, and neither
matches the CRC RNC stores for it. So this is the one tool that decodes a
format rather than running the code that decodes it, and it is held to the
format's own check instead: **every layer's output must match the CRC-16 in
its header, or nothing is written.** A decoder that is wrong anywhere fails
that; one that passes it is producing the stub's bytes.

What the format is, as this file reads it (each point measured on 1.11's
TIM.EXE, not taken on trust):

  - an 18-byte header, big-endian: "RNC", method 1, unpacked size, packed
    size, unpacked CRC, packed CRC, leeway, chunk count;
  - then a key word, little-endian, whose bit 1 is the "keyed" flag and whose
    other bits are the key: 0x664e is key 0x664c, and the inner layer's
    0xfffe is 0xffff with the flag, since a key of all ones is the only one
    that XORs every run with 0xff;
  - then the bit stream, read a 16-bit little-endian word at a time, least
    significant bit first, whose first two bits are skipped (lock and key);
  - per chunk, three Huffman tables (literal-run length, distance, match
    length): a 5-bit count and a 4-bit length per code, canonical codes
    assigned by length and stored bit-reversed; a value v >= 2 is followed
    by v - 1 extra bits;
  - then a 16-bit count of sub-chunks, each a literal run - bytes taken
    straight from the stream after the lookahead word, XORed with the key's
    low byte - and, but for the last, a copy of (length + 2) bytes from
    (distance + 1) back;
  - the key is rotated right one bit two times before the first run and once
    after every run that is not empty.

The inner layer ends with RNC's DOS stub, which carries what an EXE header
would: the entry at cs:0, the stack at cs:4, and a relocation table at
cs:0x29a that the stub walks at 19cf:(0x19e2e) - a byte count (0 ends it), a
segment word, then that many byte-sized steps of the offset within it, the
load segment added at each. That table is read here as the stub reads it.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim


def crc16(data):
    table = []
    for i in range(256):
        v = i
        for _ in range(8):
            v = (v >> 1) ^ 0xA001 if v & 1 else v >> 1
        table.append(v)
    c = 0
    for b in data:
        c = table[(c ^ b) & 0xFF] ^ (c >> 8)
    return c


class Bits:
    """The bit stream: a lookahead word at `p`, more loaded as bits go."""

    def __init__(self, data, p):
        self.d, self.p = data, p
        self.buf, self.cnt = self.word(p), 16

    def word(self, p):
        d = self.d
        return (d[p] if p < len(d) else 0) | ((d[p + 1] if p + 1 < len(d) else 0) << 8)

    def peek(self, mask):
        return self.buf & mask

    def advance(self, n):
        self.buf >>= n
        self.cnt -= n
        if self.cnt < 16:
            self.p += 2
            self.buf |= self.word(self.p) << self.cnt
            self.cnt += 16

    def read(self, mask, n):
        r = self.buf & mask
        self.advance(n)
        return r

    def refill(self):
        """After a literal run: the top word is replaced by what is at `p`."""
        self.cnt -= 16
        self.buf &= (1 << self.cnt) - 1
        self.buf |= self.word(self.p) << self.cnt
        self.cnt += 16


def mirror(x, n):
    r = 0
    for _ in range(n):
        r = (r << 1) | (x & 1)
        x >>= 1
    return r


def huffman_table(bits):
    num = bits.read(0x1F, 5)
    if not num:
        return []
    lengths = [bits.read(0x0F, 4) for _ in range(num)]
    table, code = [], 0
    for length in range(1, 17):
        for value in range(num):
            if lengths[value] == length:
                table.append((mirror(code, length), length, value))
                code += 1
        code <<= 1
    return table


def huffman_read(table, bits):
    for code, length, value in table:
        if bits.peek((1 << length) - 1) == code:
            break
    else:
        raise ValueError("no code matches at stream offset %#x" % bits.p)
    bits.advance(length)
    if value >= 2:
        v = 1 << (value - 1)
        return v | bits.read(v - 1, value - 1)
    return value


def ror16(k):
    return ((k >> 1) | ((k & 1) << 15)) & 0xFFFF


def unpack(data, off):
    """One RNC method-1 layer at `off`. Answers the bytes; raises unless the
    CRCs RNC stores for both the packed and the unpacked data hold."""
    if data[off:off + 4] != b"RNC\x01":
        raise SystemExit("no RNC method-1 header at %#x" % off)
    usize, psize = struct.unpack_from(">II", data, off + 4)
    ucrc, pcrc = struct.unpack_from(">HH", data, off + 12)
    src = data[off + 18:off + 18 + psize]
    if crc16(src) != pcrc:
        raise SystemExit("packed CRC fails at %#x" % off)
    key = struct.unpack_from("<H", src, 0)[0]
    key = 0xFFFF if key == 0xFFFE else key & ~2
    key = ror16(ror16(key))
    bits = Bits(src, 2)
    bits.advance(2)
    out = bytearray()
    while len(out) < usize:
        raw, dist, length = (huffman_table(bits), huffman_table(bits),
                             huffman_table(bits))
        runs = bits.read(0xFFFF, 16)
        while True:
            n = huffman_read(raw, bits)
            if n:
                k = key & 0xFF
                out += bytes(b ^ k for b in src[bits.p:bits.p + n])
                bits.p += n
                bits.refill()
                key = ror16(key)
            runs -= 1
            if runs <= 0:
                break
            back = huffman_read(dist, bits) + 1
            n = huffman_read(length, bits) + 2
            for _ in range(n):
                out.append(out[-back])
    out = bytes(out[:usize])
    if crc16(out) != ucrc:
        raise SystemExit("unpacked CRC fails at %#x: %04x, the header says %04x"
                         % (off, crc16(out), ucrc))
    return out


def stub_facts(layer):
    """Entry, stack and relocations out of the DOS stub that ends `layer`."""
    psize = struct.unpack_from(">I", layer, 8)[0]
    stub = (18 + psize + 15) & ~15              # the stub's segment, as an offset
    ip, cs, sp, ss = struct.unpack_from("<4H", layer, stub)
    relocs, p = [], stub + 0x29A
    while True:
        count = layer[p]
        p += 1
        if count == 0:
            break
        seg = struct.unpack_from("<H", layer, p)[0]
        p += 2
        at = 0
        for _ in range(count):
            at += layer[p]
            p += 1
            relocs.append(seg * 16 + at)
    return dict(ip=ip, cs=cs, sp=sp, ss=ss, relocs=relocs)


def recover(packed_path, verbose=True):
    raw = open(packed_path, "rb").read()
    outer = unpack(raw, raw.find(b"RNC\x01"))
    facts = stub_facts(outer)
    image = unpack(outer, 0)
    if verbose:
        print("outer layer  %d bytes, CRC good" % len(outer))
        print("image        %d bytes, CRC good" % len(image))
        print("entry        %04x:%04x" % (facts["cs"], facts["ip"]))
        print("stack        %04x:%04x" % (facts["ss"], facts["sp"]))
        print("relocations  %d" % len(facts["relocs"]))
    return image, facts


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--packed", default=tim.PACKED_EXE)
    ap.add_argument("-o", "--out", default=tim.UNPACKED_EXE)
    ap.add_argument("--image", default=tim.IMAGE)
    args = ap.parse_args()
    image, facts = recover(args.packed)
    import unlzexe
    info = dict(size=len(image), relocs=facts["relocs"], cs=facts["cs"],
                ip=facts["ip"], ss=facts["ss"], sp=facts["sp"])
    os.makedirs(os.path.dirname(args.image), exist_ok=True)
    open(args.image, "wb").write(image)
    open(args.out, "wb").write(unlzexe.build_exe(image, info, args.packed))
    print("wrote %s and %s" % (args.image, args.out))


if __name__ == "__main__":
    main()
