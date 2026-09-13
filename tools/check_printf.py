#!/usr/bin/env python3
"""Check the port's `printf` engine against the host's libc.

`vprinter` at 0x0c2ed is Borland's `__vprinter`, 1150 bytes of hand-written
assembly transcribed as C, and nothing the game does reaches it with a
conversion in the format: the four `printf`s in the image carry none, and
`sprintf` and `vsprintf` are never called. So `verify.py` can say "never
called" and nothing more. But every one of these is a libc function, whose
behaviour the standard pins, and the host's libc is a second oracle: for a
format and its arguments, `borland_vsprintf` through `libtim.so` and
`snprintf` through the host's C library have to answer the same string.

The port takes the arguments as guest words at a pointer, because the
original reads them off its caller's stack; this script lays them out in
guest memory the way that stack would hold them - a word per `int`, two per
`long`, low word first - and hands libc the same values as C ints. `%p`, `%n`
and the Borland-only `F` and `N` prefixes have no libc twin and are checked
against what the listing says instead, in the last few cases.

Every case prints its verdict; the exit status is the number that differ.
"""
import argparse
import ctypes
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import verify  # noqa: E402  the port's loader and its guest-memory helpers
import tim     # noqa: E402  where the recovered image is

DG = 0x2d3c0                    # DGROUP's image offset, and the base used here
FMT, ARGS, BUF, STR = 0x9000, 0x9100, 0x9200, 0x9800   # free DGROUP offsets;
                                                        # BUF is 512 bytes


def guest(lib):
    return (ctypes.c_ubyte * 0x100000).in_dll(lib, "guest_mem")


def put(lib, off, data):
    ctypes.memmove(ctypes.addressof(guest(lib)) + DG + off, data, len(data))


def words(*vals):
    out = b""
    for v in vals:
        out += (v & 0xffff).to_bytes(2, "little")
    return out


def port(lib, fmt, argwords):
    put(lib, FMT, fmt.encode() + b"\0")
    put(lib, ARGS, words(*argwords) + b"\0" * 8)
    put(lib, BUF, b"\xaa" * 512)
    n = lib.borland_vsprintf(verify.dgp(lib, BUF), verify.dgp(lib, FMT),
                             verify.dgp(lib, ARGS))
    raw = bytes(guest(lib))[DG + BUF:DG + BUF + 512]
    return n, raw[:raw.index(b"\0")].decode("latin-1")


def libc_expect(fmt, cargs):
    libc = ctypes.CDLL(None)
    buf = ctypes.create_string_buffer(512)
    libc.snprintf(buf, 512, fmt.encode(), *cargs)
    return buf.value.decode("latin-1")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print every case, not only the ones that differ")
    args = ap.parse_args()

    lib = verify.load_lib()
    verify.declare_restypes(lib)
    ctypes.c_uint32.in_dll(lib, "dgroup_base").value = DG
    # The class table at 0x4da1 and "(null)" at 0x4d9a are the image's.
    img = open(tim.IMAGE, "rb").read()
    put(lib, 0, img[DG:DG + 0x10000])
    put(lib, STR, b"abc\0")

    S = STR                     # a near pointer to "abc", as a guest word
    i32 = ctypes.c_int32
    u32 = ctypes.c_uint32
    cs = ctypes.c_char_p

    # (format, the guest words the caller pushed, libc's arguments)
    # A 16-bit `int` is one word; `%ld` is two, low first. libc gets the same
    # value widened, and `l` dropped, because its `int` is already 32 bits.
    cases = [
        ("hello", [], []),
        ("%d", [42], [i32(42)]),
        ("%d", [-42], [i32(-42)]),
        ("%5d|", [42], [i32(42)]),
        ("%-5d|", [42], [i32(42)]),
        ("%05d", [42], [i32(42)]),
        ("%05d", [-42], [i32(-42)]),
        ("%+d", [42], [i32(42)]),
        ("% d", [42], [i32(42)]),
        ("%+05d", [42], [i32(42)]),
        ("%-08d|", [42], [i32(42)]),
        ("%u", [-42], [u32(65494)]),
        ("%x", [0xbeef], [u32(0xbeef)]),
        ("%X", [0xbeef], [u32(0xbeef)]),
        ("%#x", [0xbeef], [u32(0xbeef)]),
        ("%#X", [0xbeef], [u32(0xbeef)]),
        ("%#x", [0], [u32(0)]),
        ("%08x", [0xbeef], [u32(0xbeef)]),
        ("%#08x", [0xbeef], [u32(0xbeef)]),
        ("%o", [8], [u32(8)]),
        ("%#o", [8], [u32(8)]),
        ("%#o", [0], [u32(0)]),
        ("%.3d", [7], [i32(7)]),
        ("%8.3d|", [7], [i32(7)]),
        ("%-8.3d|", [7], [i32(7)]),
        ("%.0d|", [0], [i32(0)]),
        ("%.0x|", [0], [u32(0)]),
        ("%*d|", [6, 42], [i32(6), i32(42)]),
        ("%-*d|", [6, 42], [i32(6), i32(42)]),
        ("%.*d", [3, 7], [i32(3), i32(7)]),
        ("%hd", [-42], [i32(-42)]),
        ("%c", [0x41], [i32(0x41)]),
        ("%s", [S], [cs(b"abc")]),
        ("%5s|", [S], [cs(b"abc")]),
        ("%-5s|", [S], [cs(b"abc")]),
        ("%.2s", [S], [cs(b"abc")]),
        ("%%", [], []),
        ("a%db%sc", [3, S], [i32(3), cs(b"abc")]),
        ("%d %x %s %c %u", [1, 255, S, 0x5a, 7],
         [i32(1), u32(255), cs(b"abc"), i32(0x5a), u32(7)]),
        ("x" * 200, [], []),
        ("%d" + "y" * 100 + "%s", [12345, S], [i32(12345), cs(b"abc")]),
    ]
    # `%ld` and friends: two words, low first; libc sees a 32-bit int.
    longs = [
        ("%ld", [0x0000, 0x0001], "%d", [i32(65536)]),
        ("%ld", [0xffff, 0xffff], "%d", [i32(-1)]),
        ("%lu", [0xffff, 0xffff], "%u", [u32(0xffffffff)]),
        ("%lx", [0xcdef, 0x89ab], "%x", [u32(0x89abcdef)]),
        ("%12ld|", [0xcdef, 0x89ab], "%12d|", [i32(-1985229329)]),
        ("%-12lu|", [0xcdef, 0x89ab], "%-12u|", [u32(0x89abcdef)]),
        ("%#lo", [0x0000, 0x0001], "%#o", [u32(65536)]),
    ]
    # No libc twin: the listing is the oracle, and the reading is beside each.
    byhand = [
        ("%s", [0], "(null)"),                      # 0x0c5ac
        ("%p", [0x1234], "1234"),                   # upper case, four digits
        ("%Fp", [0x1234, 0x5678], "5678:1234"),     # SSSS:OOOO with the F prefix
        ("%Fs", [S, 0x2d3c], "abc"),                # a far string: DGROUP's own segment
        ("%q rest %d", [1], "%q rest %d"),          # 0x0c73b: % and the rest, verbatim
        ("abc%", [], "abc%"),                       # the same at the end
        ("%+u", [42], "+42"),                       # 0x0c498 skips clearing the sign
        ("%5.0d|", [0], "|"),                       # 0x0c4e5: zero with a zero precision
                                                    # prints nothing, not even the width;
                                                    # libc would print five spaces
        ("%.5s", [S], "abc"),                       # precision past the end
        ("%-3c|", [0x41], "A  |"),
    ]

    bad = 0
    total = 0

    def check(fmt, argwords, expect, how):
        nonlocal bad, total
        total += 1
        n, got = port(lib, fmt, argwords)
        ok = got == expect and n == len(expect)
        if not ok:
            bad += 1
        if args.verbose or not ok:
            print("%s  %-28r port %r (%d)  %s %r"
                  % ("ok  " if ok else "BAD ", fmt, got, n, how, expect))

    for fmt, argwords, cargs in cases:
        check(fmt, argwords, libc_expect(fmt, cargs), "libc")
    for fmt, argwords, cfmt, cargs in longs:
        check(fmt, argwords, libc_expect(cfmt, cargs), "libc")
    for fmt, argwords, expect in byhand:
        check(fmt, argwords, expect, "listing")

    print("%d of %d formats agree with %s" % (total - bad, total,
          "libc and the listing" if bad == 0 else "the oracle"))
    return bad


if __name__ == "__main__":
    sys.exit(main())
