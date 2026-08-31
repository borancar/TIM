"""Capture reference frames from the original, on the guest's own cue.

The cue is the **page flip**: the video driver makes a composed frame visible
by writing the high byte of the CRTC start address, and that instant is the
only moment a frame is complete and not half-drawn. Capturing on a wall clock
or a display-frame number instead would land somewhere different on every
machine, which is exactly what makes a comparison unreproducible.

A capture carries the palette *indices*, not a picture. Two different indices
can share a colour, and a comparison that comes down to RGB cannot see the
difference; the PNG beside it is for looking at, never for measuring.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
import png
import drive
import zlib

MAGIC = b"TIMSCRN1"


def write_scrn(path, w, h, svb, palette, indices):
    with open(path, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<HHH", w, h, 0xFFFF if svb is None else svb))
        pal = bytearray(768)
        for i, (r, g, b) in enumerate(palette[:256]):
            pal[i * 3:i * 3 + 3] = bytes((r, g, b))
        f.write(bytes(pal))
        f.write(bytes(indices))


def read_scrn(path):
    d = open(path, "rb").read()
    assert d[:8] == MAGIC, "%s is not a capture" % path
    w, h, svb = struct.unpack_from("<HHH", d, 8)
    pal = [tuple(d[14 + i * 3:14 + i * 3 + 3]) for i in range(256)]
    idx = d[14 + 768:14 + 768 + w * h]
    return w, h, (None if svb == 0xFFFF else svb), pal, idx


def capture_flips(instructions, wanted, outdir, prefix, every=0,
                  step=drive.DEFAULT_STEP, ips=drive.DEFAULT_IPS, verbose=True,
                  png_too=True, digests=None, snapshot=None, clicks=(),
                  moves=()):
    """Run the game, capturing the frame made visible by each chosen flip.

    With `digests` naming a file, every flip contributes **one line** - its
    number, a CRC-32 of the frame, the blanking line and the start address -
    and no pixels are written at all. That is what a comparison against the
    port needs: which flips differ. Whole frames are then worth taking for
    those flips and no others, which is what `--flip N` is for.

    Writing every frame instead cost five gigabytes to establish that none of
    them differed. The digest file for the same run is a few hundred kilobytes.
    """
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    # `snapshot` starts from a saved state instead of the program's entry
    # point, so a screen thirty seconds into the game can be captured without
    # replaying the thirty seconds - and the flip numbers are then counted from
    # the snapshot, which is what the port's own run has to be lined up with.
    m = drive.machine(ips=ips, snapshot=snapshot)
    dig = open(digests, "w") if digests else None
    if dig is None:
        os.makedirs(outdir, exist_ok=True)
    state = {"flips": 0, "saved": 0}
    want = set(wanted or ())
    if dig is not None and not want and not every:
        every = 1                      # digests are cheap; take them all

    def on_out(uc, port, size, value, ud):
        # CRTC index 0x0C, the start address high byte: a 16-bit write puts
        # the index in the low byte and the data in the high byte. That write
        # is the instant a composed frame becomes visible, and the only
        # instant at which it is complete rather than half drawn.
        if port != 0x3D4 or size != 2:
            return
        index, data = value & 0xFF, (value >> 8) & 0xFF
        if index != 0x0C:
            return
        n = state["flips"]
        state["flips"] = n + 1

        # The same clicks and pointer moves the port's TIM_CLICK and
        # TIM_POINTER make, at the same flips. A whole-frame comparison is only
        # honest if both sides were given the same input from the same start:
        # driving the port to a screen with a click and the original to it with
        # a snapshot leaves the two with different histories, and the pointer's
        # saved backdrop is then different in a way no amount of transcription
        # will fix.
        for at, cx, cy in clicks:
            if n == at:
                m.click_mouse(0, cx, cy, True)
            elif n == at + 2:
                m.click_mouse(0, cx, cy, False)
        for at, cx, cy in moves:
            if n == at:
                m.mouse_pos = (cx, cy)

        if not (n in want or (every and n % every == 0)):
            return
        # The emulator's own port hook is registered first, so start_addr is
        # already the new one by the time this runs. Checked, not assumed.
        expect = data << 8
        if m.start_addr != expect:
            raise SystemExit("flip %d: start_addr %#06x but wrote %#06x"
                             % (n, m.start_addr, expect))
        fb = m.framebuffer()

        if dig is not None:
            dig.write("%d %08x %s %#06x\n"
                      % (n, zlib.crc32(bytes(fb)) & 0xFFFFFFFF,
                         m.start_vertical_blank(), m.start_addr))
            dig.flush()
            state["saved"] += 1
            return

        base = os.path.join(outdir, "%s%04d" % (prefix, n))
        write_scrn(base + ".scrn", m.width, m.height,
                   m.start_vertical_blank(), m.palette, fb)
        if png_too:
            png.save_indexed(base + ".png", fb, m.width, m.height, m.palette)
        state["saved"] += 1
        if verbose:
            nz = sum(1 for b in fb if b)
            print("  flip %4d start=%#06x svb=%s nonzero=%-6d -> %s.scrn"
                  % (n, m.start_addr, m.start_vertical_blank(), nz, base),
                  flush=True)

    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)

    stop_at = max(want) + 1 if want else None

    def on_slice(mm, done):
        return stop_at is not None and state["flips"] > stop_at

    why = drive.drive(m, instructions, step=step, on_slice=on_slice)
    print("%d flips seen, %d captured%s"
          % (state["flips"], state["saved"], ("  (%s)" % why) if why else ""))
    return state


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--insns", type=int, default=120_000_000,
                    help="emulated instruction budget")
    ap.add_argument("--flip", type=int, action="append", default=[],
                    help="capture the frame this page flip makes visible")
    ap.add_argument("--every", type=int, default=0,
                    help="capture every Nth flip, for surveying")
    ap.add_argument("--digests", default=None, metavar="FILE",
                    help="write one line per flip - number, CRC-32 of the "
                         "frame, blanking line, start address - and no pixels. "
                         "The way to compare a whole run against the port; use "
                         "--flip for the few frames a side-by-side needs")
    ap.add_argument("--click", action="append", default=[],
                    metavar="FLIP:X:Y",
                    help="press and release the left button there, at that "
                         "flip - the same thing the port's TIM_CLICK does, so "
                         "the two runs can be given the same input")
    ap.add_argument("--move", action="append", default=[], metavar="FLIP:X:Y",
                    help="move the pointer there at that flip, as TIM_POINTER")
    ap.add_argument("--from", dest="snapshot", default=None, metavar="SNAP",
                    help="start from a tools/snapshot.py state, not the entry "
                         "point; flips are numbered from there")
    ap.add_argument("--out", default="out/ref")
    ap.add_argument("--prefix", default="flip")
    ap.add_argument("--no-png", action="store_true",
                    help="the .scrn only. A lockstep comparison reads the "
                         "indices and never the picture, and over hundreds of "
                         "flips the PNGs cost more than the capture does")
    args = ap.parse_args()
    capture_flips(args.insns, args.flip, args.out, args.prefix,
                  every=args.every, png_too=not args.no_png,
                  digests=args.digests, snapshot=args.snapshot,
                  clicks=[tuple(int(v, 0) for v in c.split(":"))
                          for c in args.click],
                  moves=[tuple(int(v, 0) for v in c.split(":"))
                         for c in args.move])


if __name__ == "__main__":
    main()
