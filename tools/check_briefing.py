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


def need_devtim():
    """The developer binary, or a sentence saying how to get one.

    `subprocess` raises `FileNotFoundError` with a path and no reason, which on
    a clean tree is a traceback about a file the reader has never heard of.
    `verify.py` already says "run `make libtim.so`" for its own missing
    artefact; this is the same courtesy for `devtim`.
    """
    path = os.path.join(ROOT, "reconstruct", "devtim")
    if not os.path.exists(path):
        raise SystemExit("no %s - run `make -C reconstruct devtim`. It is the "
                         "developer binary, and the flags this needs live "
                         "there rather than in what ships." % path)
    return path

# **Where the coordinates come from.** Every click below is the middle of a
# screen region, and the regions are the table `screen_regions` in
# `reconstruct/seg0000.c`: `+6` and `+0x0a` are the x range, `+8` and `+0x0c`
# the y range, and `+0x10` the mode the region switches to - 0x400 for freeform,
# 0x100 for Load Machine, 0x80 for Save. A change to that table moves these,
# and the symptom is not an error but a screen that does not match, so the
# provenance is written here rather than left to be rediscovered.
#
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
    # **The game screen**, which is `game_screen_loop` and nothing else - two
    # clicks in: dismiss the copy protection, then the panel's play triangle.
    # Everything the briefing already proves is upstream of this; what is new
    # here is the loop itself, its five deferred redraws, and the regions and
    # movers it drives.
    #
    # The flips are late because the second click is at 400 and the screen has
    # to settle after it.
    #
    # **This screen does not settle, and cannot be a zero-pixel check.** It
    # comes out at about 2,700 of 307,200 pixels, 0.85%, and the difference is
    # in exactly two places:
    #
    #   2,592  the odometer strip, y 380..445 - the score and bonus reels,
    #          which turn on the timer, and the two sides do not pace their
    #          timers alike. The same reason the copy-protection page number
    #          differs; see STATUS.md.
    #     108  a 22 by 7 box at 78,118 - the mouse cursor.
    #
    # The palettes are identical and nothing outside those two boxes differs,
    # which is the useful part: the play area, the frame, the parts bin and the
    # panel are pixel-for-pixel the same. Read this screen as "did anything
    # move outside the reels and the pointer", not as a pass or a fail.
    "level": {
        "clicks": [(200, 320, 200), (400, 78, 105)],
        "flips": (560, 580, 600),
        "insns": 400_000_000,
    },
    # **Saving a machine**, which is the write path end to end: create the
    # file, open it for writing, truncate it, fill the stream buffer, flush it
    # and close it. Seven clicks - the four above but on Save Machine, then a
    # row of the listing to fill the name field, SAVE, and YES to the overwrite
    # question. The frames compared are after it has returned to the panel.
    #
    # The port writes into an in-memory overlay and never onto the host, which
    # is what the emulator does too, so running this leaves the game directory
    # exactly as it found it.
    "save": {
        "clicks": [(200, 320, 200), (420, 76, 152), (560, 222, 220),
                   (700, 220, 152), (840, 100, 128), (980, 88, 312),
                   (1120, 222, 220)],
        "flips": (1200, 1250),
        "insns": 700_000_000,
    },
}


def run_port(outdir, flip, timeout, clicks, wanted):
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
    # **`TIM_FLIPWANT`, not just the stop.** `TIM_FLIPS=<dir>:<last>` writes a
    # frame for *every* flip up to `<last>` - 308 KB each, so a run to flip 800
    # leaves a quarter of a gigabyte behind. This comparison reads three of
    # them. Naming the three is the difference between 900 KB and 250 MB, and
    # /tmp has twice been filled by the other reading.
    # TIM_HEADLESS because `devtim` now opens a window by default, as `tim`
    # does. A batch comparison must not need a display, and it reads the
    # planes rather than the window either way.
    env = dict(os.environ, TIM_HEADLESS="1",
               TIM_CLICK=",".join("%d:%d:%d" % c for c in clicks),
               TIM_FLIPWANT=",".join(str(f) for f in wanted),
               TIM_FLIPS="%s:%d" % (outdir, flip))
    # **The port's own stderr is kept.** It says what is wrong when it cannot
    # start - "cannot read out/TIM.img ... run tools/unlzexe.py first" - and
    # discarding it turns a missing input into "the port never reached it",
    # which sends the reader to debug the port instead of running one command.
    log = open(os.path.join(outdir, "port.log"), "wb")
    proc = subprocess.Popen([need_devtim()],
                            cwd=ROOT, env=env,
                            stdout=subprocess.DEVNULL,
                            stderr=log)
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
    run_port(port_dir, last, args.timeout, screen["clicks"], flips)

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
            # If the port is the one that did not, say what it said. It refuses
            # to start with a sentence naming the command to run, and that is
            # far more use than the flip number.
            if "port" in missing:
                why = os.path.join(port_dir, "port.log")
                tail = (open(why, "rb").read()[-400:].decode("utf-8", "replace")
                        if os.path.exists(why) else "")
                for line in tail.strip().splitlines()[-3:]:
                    print("  port said: %s" % line)
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
