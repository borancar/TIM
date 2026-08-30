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
                  png_too=True):
    """Run the game, capturing the frame made visible by each chosen flip."""
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    m = drive.machine(ips=ips)
    os.makedirs(outdir, exist_ok=True)
    state = {"flips": 0, "saved": 0}
    want = set(wanted or ())

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
        if not (n in want or (every and n % every == 0)):
            return
        # The emulator's own port hook is registered first, so start_addr is
        # already the new one by the time this runs. Checked, not assumed.
        expect = data << 8
        if m.start_addr != expect:
            raise SystemExit("flip %d: start_addr %#06x but wrote %#06x"
                             % (n, m.start_addr, expect))
        fb = m.framebuffer()
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
    ap.add_argument("--out", default="out/ref")
    ap.add_argument("--prefix", default="flip")
    ap.add_argument("--no-png", action="store_true",
                    help="the .scrn only. A lockstep comparison reads the "
                         "indices and never the picture, and over hundreds of "
                         "flips the PNGs cost more than the capture does")
    args = ap.parse_args()
    capture_flips(args.insns, args.flip, args.out, args.prefix,
                  every=args.every, png_too=not args.no_png)


if __name__ == "__main__":
    main()
