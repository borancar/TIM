"""Prove the level-one briefing still matches, from the entry point.

The result this exists to protect: the port and the original, both started at
the program's own entry point and given the same click at the same flip, draw
the level-one briefing **identically** - zero differing pixels out of 307,200.

It is a check rather than a paragraph in STATUS.md because a number in prose
rots quietly. This runs both sides and compares them, and says which flip it
compared and how many pixels differed.

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

CLICK_FLIP, CLICK_X, CLICK_Y = 200, 320, 200
# Flips to compare. All of them are after the briefing has finished painting -
# 207 is the first, and every flip from there was measured identical - so this
# is three chances to catch a difference rather than one, at no extra cost:
# both sides are run once and the frames are already written.
SETTLED = (210, 230, 260)
INSNS = 150_000_000            # enough to reach it from the entry point


def run_port(outdir, flip, timeout):
    """Run the port until it has written the frame, then stop it.

    A DOS game does not exit - it shows its menu and waits - so the port has to
    be stopped from outside. Waiting out a fixed timeout works and wastes all
    of it; the frame is usually there in seconds. So this polls for the file
    and for it to have stopped growing, and only falls back on the timeout if
    the flip is never reached.
    """
    want = os.path.join(outdir, "flip%04d.scrn" % flip)
    env = dict(os.environ,
               SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
               TIM_CLICK="%d:%d:%d" % (CLICK_FLIP, CLICK_X, CLICK_Y),
               TIM_FLIPS="%s:%d" % (outdir, flip))
    proc = subprocess.Popen([os.path.join(ROOT, "reconstruct", "tim")],
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


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--flip", type=int, action="append", default=[],
                    help="which flips to compare; may be repeated")
    ap.add_argument("--insns", type=int, default=INSNS)
    ap.add_argument("--timeout", type=int, default=180)
    ap.add_argument("--keep", default=None,
                    help="where to leave the two frames; a temp dir otherwise")
    args = ap.parse_args()

    flips = sorted(args.flip) or list(SETTLED)
    last = flips[-1]
    out = args.keep or tempfile.mkdtemp(prefix="briefing")
    ref_dir = os.path.join(out, "ref")
    port_dir = os.path.join(out, "port")
    os.makedirs(ref_dir, exist_ok=True)
    os.makedirs(port_dir, exist_ok=True)

    print("port: running to flip %d ..." % last, flush=True)
    run_port(port_dir, last, args.timeout)

    print("original: running to flips %s ..."
          % ", ".join(str(f) for f in flips), flush=True)
    capture.capture_flips(args.insns, flips, ref_dir, "flip",
                          png_too=False, verbose=False,
                          clicks=[(CLICK_FLIP, CLICK_X, CLICK_Y)])

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
