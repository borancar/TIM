"""Exercise every part in free-design mode: place it, run it, flip it, run it.

Free design is the only screen with **every** part in its bin, and no puzzle to
solve - the machine runs until you stop it. So it is where a part's hooks can
be reached deliberately rather than by finding the one puzzle that happens to
use it, which is how the bellows, the conveyor and kind 31 were each found
hours apart.

For each part in the bin this drives the sequence Boran described: take it out
of the bin, flip it both ways with the X and Y keys, put it in the play area,
start the machine, stop it, pick the part up again and drop it back in the bin.
That reaches all six of a kind's hooks - the setup when it is taken out, the
flip on each key, the step and the hit tests while the machine runs, the drive
when something is connected to it, and the settle when a drag ends.

**The clicks and keys are the game's own**, read out of the editor's region
table and the game's scancode tables, so the same sequence would drive the
original. Coordinates:

  * the five bin slots, x 576..632, at y 100..144, 145..196, 197..248,
    249..300 and 301..352 - the regions whose click handler is 0f0f:2e24;
  * the bin's forward arrow at 608..639 x 67..90, which pages it on by five;
  * the box above the bin, 576..632 x 0..63, which starts the machine;
  * a left click anywhere stops it, which `run_machine_loop` does at 0x012ab.

Scancodes 45 and 21 are X and Y, the two flip axes, from the table
`part_key_shortcut` searches at CS:0x2650.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import concurrent.futures
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DGROUP = 0x2E4C0
MEM_AT = 12

SLOT_Y = (122, 170, 222, 274, 326)      # centres of the five bin regions
SLOT_X = 604
PER_PAGE = len(SLOT_Y)
BIN_FORWARD = (620, 78)
START_MACHINE = (604, 30)
DROP = (300, 200)                       # somewhere in the play area
ELSEWHERE = (200, 320)                  # a click that only stops the machine
KEY_X, KEY_Y = 45, 21


def bin_kinds(path):
    """The kind of each part in the bin, walked from the list head at 0x50d7."""
    d = open(path, "rb").read()
    if d[:8] != b"TIMPORT1":
        raise SystemExit("%s is not a port snapshot" % path)
    base = MEM_AT + DGROUP

    def u16(off):
        return d[base + off] | d[base + off + 1] << 8

    out, si, seen = [], u16(0x50D7), set()
    while si and si not in seen and len(out) < 200:
        seen.add(si)
        out.append(u16(si + 4))
        si = u16(si)
    return out


def plan(index):
    """Clicks, keys and the flip to stop at, for the bin part at `index`."""
    downs, slot = divmod(index, PER_PAGE)

    at = 4
    clicks = []
    for _ in range(downs):
        clicks.append((at, BIN_FORWARD))
        at += 26

    at += 26
    clicks.append((at, (SLOT_X, SLOT_Y[slot])))     # take it out of the bin
    keys = [(at + 20, KEY_X), (at + 34, KEY_Y)]     # flip both axes in hand
    clicks.append((at + 50, DROP))                  # put it down
    clicks.append((at + 70, START_MACHINE))         # run
    clicks.append((at + 150, ELSEWHERE))            # stop
    clicks.append((at + 180, DROP))                 # pick it up again
    clicks.append((at + 200, (SLOT_X, SLOT_Y[slot])))   # drop it in the bin
    return clicks, keys, at + 240


def run_one(index, kind, start, binary, seconds):
    clicks, keys, stop = plan(index)
    env = dict(os.environ,
               TIM_HEADLESS="1",
               TIM_CLICK=",".join("%d:%d:%d" % (f, x, y) for f, (x, y) in clicks),
               TIM_KEY=",".join("%d:%d" % (f, k) for f, k in keys),
               TIM_STOPFLIP="%d" % stop)
    try:
        p = subprocess.run([binary, "--restore", start], cwd=ROOT, env=env,
                           capture_output=True, text=True, timeout=seconds)
        err, code = p.stderr or "", p.returncode
    except subprocess.TimeoutExpired as e:
        err = (e.stderr or b"").decode("utf-8", "replace") \
              if isinstance(e.stderr, bytes) else (e.stderr or "")
        code = "timeout"

    trap = None
    m = re.search(r"reached ([^\n]*), which is not transcribed yet", err)
    if m:
        trap = m.group(1)
    elif "not transcribed" in err:
        trap = err.split("not transcribed")[0].strip().splitlines()[-1][:60]
    return index, kind, trap, code


def main():
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        epilog="reads no environment of its own; it sets TIM_HEADLESS, "
               "TIM_CLICK, TIM_KEY and TIM_STOPFLIP for each devtim it runs",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--from-snapshot", default="out/devtim006.snap",
                    help="a port snapshot in free design (default: %(default)s)")
    ap.add_argument("--first", type=int, default=0)
    ap.add_argument("--last", type=int, default=-1,
                    help="last bin index; -1 means every part in the bin")
    ap.add_argument("--only", help="comma-separated bin indices instead")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--seconds", type=int, default=300)
    args = ap.parse_args()

    binary = os.path.join(ROOT, "reconstruct", "devtim")
    if not os.path.exists(binary):
        raise SystemExit("no %s - run `make -C reconstruct devtim`" % binary)

    kinds = bin_kinds(args.from_snapshot)
    last = args.last if args.last >= 0 else len(kinds) - 1
    want = ([int(x, 0) for x in args.only.split(",")] if args.only
            else list(range(args.first, min(last, len(kinds) - 1) + 1)))

    rows = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futs = [pool.submit(run_one, i, kinds[i], args.from_snapshot, binary,
                            args.seconds) for i in want]
        for f in concurrent.futures.as_completed(futs):
            rows.append(f.result())

    rows.sort()
    traps = {}
    for index, kind, trap, code in rows:
        if trap:
            traps.setdefault(trap, []).append(kind)
        print("  bin %2d  kind %2d  %s" % (index, kind, trap or "ok"))

    clean = sum(1 for r in rows if not r[2])
    print("\n%d of %d parts placed, flipped and run with no trap"
          % (clean, len(rows)))
    if traps:
        print("\n%d distinct traps:" % len(traps))
        for what, ks in sorted(traps.items(), key=lambda kv: -len(kv[1])):
            print("  %-46s kinds %s"
                  % (what, ", ".join(str(k) for k in ks)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
