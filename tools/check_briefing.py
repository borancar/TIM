"""Prove a screen still matches the original, from the entry point.

The result this exists to protect: the port and the original, both started at
the program's own entry point and given the same clicks at the same flips, draw
the same screen **identically** - zero differing pixels out of 307,200.

Two screens so far, `--screen briefing` and `--screen picker`. The briefing is
one click in; the picker is four, through freeform mode. Both are driven from
the entry point rather than from a snapshot, so the two sides have the same
history and the pointer's saved backdrop is the same on each.

It is a check rather than a paragraph in STATUS.md because a number in prose
rots quietly. This runs both sides and compares them, and says which flip it
compared and how many pixels differed. It earns its keep: the picker
comparison found `pick_file`'s arguments in the wrong order, which the port's
own code read perfectly well either way.

**Not the copy-protection screen.** Flips 202 to 206 are expected to differ and
to differ differently each run: the screen names a manual page taken from the
timer's countdown at DGROUP 0x44ef, the reference paces its timer on emulated
instructions and the port paces its on the host clock, so the two arrive there
having taken very different numbers of ticks. Measured over four runs the port's
page came out 6, 7, 12 and 12. That is a property of the two clocks, not a
transcription fault, and this tool does not pretend otherwise - it compares a
flip after the briefing has settled.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capture

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A screen is a list of clicks, the flips to compare, and how many instructions
# the original needs to reach them. Both sides get the *same* clicks at the same
# flips - a click is placed at a flip because that is the one clock the port and
# the emulator already agree on.
#
# The flips chosen are always after the screen has finished painting, and there
# are three of them rather than one: both sides are run once and the frames are
# already written, so two more comparisons cost nothing and catch a difference
# that happens to be absent from the first.
SCREENS = {
    # The level-one briefing. One click dismisses the copy-protection screen;
    # 207 is the first settled flip and every flip from there was identical.
    "briefing": {
        "clicks": [(200, 320, 200)],
        "flips": (210, 230, 260),
        "insns": 150_000_000,
    },
    # The **file picker**, which is four clicks in: dismiss, the wrench to ask
    # for freeform mode, YES to confirm it, then Load Machine. It is behind
    # freeform because `screen_state_0100` returns at once outside it.
    #
    # This is the check that caught `pick_file`'s argument order. The pattern is
    # pushed last and is therefore the third argument; the port had it first,
    # copied an empty string, built no extension filter, and listed every file
    # in the directory where the original listed three. The code read correctly
    # either way and only a side-by-side could tell.
    "picker": {
        "clicks": [(200, 320, 200), (420, 76, 152),
                   (560, 222, 220), (700, 170, 152)],
        "flips": (740, 760, 790),
        "insns": 400_000_000,
    },
}


def run_port(outdir, flip, timeout, clicks):
    """Run the port until it has written the frame, then stop it.

    **`devtim`, not `tim`.** The flags this needs live in `devdump.c`, which
    links only into the developer binary - the Makefile's rule is that nothing a
    measurement depends on may reach what ships, and for a long time it did,
    because `devdump.c` was in the object list both binaries share. `devtim` has
    no SDL either, so there is no dummy video driver to arrange: the frame this
    reads is composed from the planes on the guest's own page flip, which is the
    same frame the window would have shown.

    A DOS game does not exit - it shows its menu and waits - so the port has to
    be stopped from outside. Waiting out a fixed timeout works and wastes all
    of it; the frame is usually there in seconds. So this polls for the file
    and for it to have stopped growing, and only falls back on the timeout if
    the flip is never reached.
    """
    want = os.path.join(outdir, "flip%04d.scrn" % flip)
    env = dict(os.environ,
               TIM_CLICK=",".join("%d:%d:%d" % c for c in clicks),
               TIM_FLIPS="%s:%d" % (outdir, flip))
    proc = subprocess.Popen([os.path.join(ROOT, "reconstruct", "devtim")],
                            cwd=ROOT, env=env,
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    try:
        deadline = time.time() + timeout
        size = -1
        while time.time() < deadline:
            if proc.poll() is not None:
                return
            if os.path.exists(want):
                now = os.path.getsize(want)
                if now == size and now > 0:
                    return              # written and no longer growing
                size = now
            time.sleep(0.25)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()


def emulator_reads_words():
    """Does the emulator satisfy a **multi-byte** read from video memory?

    It used not to. `_on_plane_read` served only the byte at the read's address
    and ignored the size, so the high byte of every `rep movsw` out of video
    memory came back from unicorn's flat memory - where nothing is ever written,
    because writes are shadowed into four planes instead. The game saves the
    rectangle under its mouse pointer that way, so the *reference* left a trail
    of black wherever the screen underneath was solid, and this comparison
    reported 452 differing pixels with the port drawing it correctly.

    The fix is upstream. Until the pin moves, a reinstall silently puts the
    fault back - and the only symptom would be this check failing again, with
    nothing to say why. So it is tested for, by calling the read hook with a
    size of two and seeing whether two bytes come back.

    Ours, and it is a check on the *reference*, which is the thing that has no
    other check on it.
    """
    import tim
    m = tim.TimMachine(tim.UNPACKED_EXE)
    # The hook returns early on a chain-4 machine that is not in a planar
    # 16-colour mode, and a machine that has not run yet is exactly that. The
    # game's mode is 0x12; say so, or the probe measures the early return.
    m.chain4 = False
    m.mode = 0x12
    base = 0xA0000
    off = 0x4000
    for p_, v in enumerate((0x11, 0x22, 0x33, 0x44)):
        m.planes[p_][off] = v
        m.planes[p_][off + 1] = v ^ 0xFF
    m.gc[4] = 0                       # read map select: plane 0
    m.gc[5] = 0                       # read mode 0
    m.uc.mem_write(base + off, b"\x00\x00")
    m._on_plane_read(m.uc, 0, base + off, 2, 0, None)
    got = bytes(m.uc.mem_read(base + off, 2))
    return got == bytes((0x11, 0x11 ^ 0xFF))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--screen", default="briefing",
                    choices=sorted(SCREENS),
                    help="which screen to compare")
    ap.add_argument("--flip", type=int, action="append", default=[],
                    help="which flips to compare; may be repeated")
    ap.add_argument("--insns", type=int, default=None)
    ap.add_argument("--timeout", type=int, default=180)
    ap.add_argument("--keep", default=None,
                    help="where to leave the two frames; a temp dir otherwise")
    args = ap.parse_args()

    screen = SCREENS[args.screen]
    flips = sorted(args.flip) or list(screen["flips"])
    insns = args.insns or screen["insns"]
    last = flips[-1]
    out = args.keep or tempfile.mkdtemp(prefix=args.screen)
    ref_dir = os.path.join(out, "ref")
    port_dir = os.path.join(out, "port")
    os.makedirs(ref_dir, exist_ok=True)
    os.makedirs(port_dir, exist_ok=True)

    if not emulator_reads_words():
        print("the emulator does not satisfy multi-byte reads from video "
              "memory.\n"
              "Every `rep movsw` out of it returns every second byte as zero, "
              "which corrupts\n"
              "the rectangle the game saves under its mouse pointer - so the "
              "*reference* will be\n"
              "wrong, by about 452 pixels, and the port will look like the one "
              "at fault.\n"
              "See STATUS.md, 'The emulator pin'.")
        return 2

    print("port: running to flip %d ..." % last, flush=True)
    run_port(port_dir, last, args.timeout, screen["clicks"])

    print("original: running to flips %s ..."
          % ", ".join(str(f) for f in flips), flush=True)
    capture.capture_flips(insns, flips, ref_dir, "flip",
                          png_too=False, verbose=False,
                          clicks=screen["clicks"])

    bad = 0
    for flip in flips:
        name = "flip%04d.scrn" % flip
        a, b = os.path.join(ref_dir, name), os.path.join(port_dir, name)
        missing = [who for p, who in ((a, "original"), (b, "port"))
                   if not os.path.exists(p)]
        if missing:
            print("flip %d: %s never reached it" % (flip, " and ".join(missing)))
            bad += 1
            continue

        w, h, svb, _, ref = capture.read_scrn(a)
        w2, h2, svb2, _, got = capture.read_scrn(b)
        if (w, h) != (w2, h2):
            print("flip %d: frames are different sizes, %dx%d and %dx%d"
                  % (flip, w, h, w2, h2))
            bad += 1
            continue

        differ = sum(1 for i in range(w * h) if ref[i] != got[i])
        print("flip %d, %dx%d, blanking at %s and %s: **%d of %d pixels differ**"
              % (flip, w, h, svb, svb2, differ, w * h))
        if differ:
            bad += 1

    if bad:
        print("the frames are in %s" % out)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
