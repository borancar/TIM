"""Prove the machine the game saves is byte for byte the original's.

**A machine file never reaches a pixel.** The screen comparisons in
`check_briefing.py` prove the picker, the panel and the dialogs; they say
nothing at all about the writer, which could get every field wrong and still
leave the same screen behind. Verifying the writer's routines one at a time -
`tools/verify.py --only sub_12430,part_index,...` - proves each of them, and
this proves what they produce *together*: the ordering across routines, the
counts written before each list, the header.

Both sides are driven from the program's entry point with the same clicks at
the same page flips, so neither is started from a state the other did not reach
the same way.

**Neither side writes a real file.** The port satisfies guest writes from an
in-memory overlay and the emulator does the same; this reads the bytes out of
each as they are closed. Running it leaves the game directory exactly as it
found it.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Dismiss the copy-protection screen, the wrench to ask for freeform, YES to
# confirm it, Save Machine, a row of the listing to fill in the name, SAVE, and
# YES to the overwrite question.
CLICKS = [(200, 320, 200), (420, 76, 152), (560, 222, 220),
          (700, 220, 152), (840, 100, 128), (980, 88, 312), (1120, 222, 220)]
# Enough to reach the save; the run stops as soon as the file is closed, so
# this is only the point at which to give up.
INSNS = 400_000_000


def run_port(outdir, timeout):
    """Run the port with `TIM_SAVEDIR` and let it write whatever it saves.

    `devtim`, because `TIM_SAVEDIR` lives in `devdump.c` and the Makefile's rule
    is that nothing a comparison depends on may reach what ships.

    A DOS game does not exit, so the port has to be stopped from outside.
    Waiting out the timeout works and wastes all of it; the file is there within
    a couple of minutes and nothing happens afterwards that this reads. So this
    polls for a file to appear and to stop growing, and falls back on the
    timeout only if the save is never reached.
    """
    env = dict(os.environ,
               TIM_CLICK=",".join("%d:%d:%d" % c for c in CLICKS),
               TIM_SAVEDIR=outdir)
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
            names = os.listdir(outdir)
            if names:
                now = sum(os.path.getsize(os.path.join(outdir, n))
                          for n in names)
                if now == size:
                    return              # written and no longer growing
                size = now
            time.sleep(0.5)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()


def run_reference(insns):
    """Run the original and answer every file it wrote, as name -> bytes.

    The emulator holds an open file's bytes on the handle and only copies them
    into its overlay for a file it *created* - overwriting one that already
    exists on the host drops them at close, which is the same thing the port
    does and is exactly the save this checks. So the handles are read on every
    slice rather than at the end: the last state seen before a handle goes away
    is what was written.
    """
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    m = drive.machine()
    state = {"flips": 0}
    written = {}

    def on_out(uc, port, size, value, ud):
        if port != 0x3D4 or size != 2 or (value & 0xFF) != 0x0C:
            return
        n = state["flips"]
        state["flips"] = n + 1
        for at, cx, cy in CLICKS:
            if n == at:
                m.mouse_input(cx, cy, 1)
            elif n == at + 2:
                m.mouse_input(cx, cy, 0)

    live = set()

    def on_slice(mm, done):
        """Take a copy of every written handle, and stop once one is closed.

        **The close is the moment the file is finished**, and it is also the
        last moment its bytes exist - the emulator drops them for a handle with
        no overlay key, which is what overwriting a host file gives you. So the
        copy is taken while the handle is open and the run ends when it goes
        away, rather than running on to a budget that was only ever a guess at
        how far the save was.

        **This is safe only because a slice is small.** `drive.DEFAULT_STEP` is
        2000 instructions and a save is many times that, so a handle cannot be
        opened, written and closed between two of these. At a slice of a
        million it could, and the file would be reported missing rather than
        wrong - which is a worse failure than a mismatch, because it looks like
        the original never saved.
        """
        now = set()
        for hn, h in list(mm.handles.items()):
            if getattr(h, "written", 0):
                leaf = h.path.replace("/", "\\").rsplit("\\", 1)[-1].upper()
                written[leaf] = bytes(h.data)
                now.add(hn)

        closed = live - now
        live.clear()
        live.update(now)
        return bool(closed)

    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, insns, on_slice=on_slice)

    for key, blob in m.overlay.items():
        written[key.rsplit("\\", 1)[-1].upper()] = bytes(blob)

    return written


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--insns", type=int, default=INSNS)
    ap.add_argument("--timeout", type=int, default=180)
    ap.add_argument("--keep", default=None,
                    help="where to leave the port's files; a temp dir "
                         "otherwise")
    args = ap.parse_args()

    out = args.keep or tempfile.mkdtemp(prefix="save")
    os.makedirs(out, exist_ok=True)

    print("port: running ...", flush=True)
    run_port(out, args.timeout)

    print("original: running ...", flush=True)
    ref = run_reference(args.insns)

    got = sorted(os.listdir(out))
    if not got:
        print("the port wrote nothing")
        return 1

    bad = 0
    for name in got:
        mine = open(os.path.join(out, name), "rb").read()
        theirs = ref.get(name)

        if theirs is None:
            print("%s: the port wrote %d bytes, the original wrote no such "
                  "file" % (name, len(mine)))
            bad += 1
            continue

        if mine == theirs:
            print("%s: **%d bytes, identical**" % (name, len(mine)))
            continue

        bad += 1
        if len(mine) != len(theirs):
            print("%s: the port wrote %d bytes, the original %d"
                  % (name, len(mine), len(theirs)))
        n = min(len(mine), len(theirs))
        first = next((i for i in range(n) if mine[i] != theirs[i]), n)
        differ = sum(1 for i in range(n) if mine[i] != theirs[i])
        print("%s: %d of %d shared bytes differ, first at %d "
              "(port %02x, original %02x)"
              % (name, differ, n, first,
                 mine[first] if first < len(mine) else 0,
                 theirs[first] if first < len(theirs) else 0))

    for name in sorted(ref):
        if name not in got:
            print("%s: the original wrote %d bytes, the port wrote no such "
                  "file" % (name, len(ref[name])))
            bad += 1

    if bad:
        print("the port's files are in %s" % out)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
