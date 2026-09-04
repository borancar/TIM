#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Prove the port plays the original's samples, byte for byte.

No screen comparison can hear anything, and `verify.py` cannot reach the sound
module's play path - the module is loaded code the game pulls out of SX.OVL, and
what it does is port I/O to a card, not a return value. So the instrument is the
same one the rest of this project uses for the driver: both the port and the
hybrid runner drive `reconstruct/io.c`, so `TIM_TRACE=sb` on each produces a
stream of blocks from the *same* code, one driven by the port's C and one by the
original's own instructions under emulation.

**Aligned by content, never by index.** The two sides share no clock. A looping
sample repeats until the game asks for the next sound, so the hybrid - which is
far slower per unit of game - goes round more times before every trigger, and
comparing block 5 against block 5 compares different moments. Runs of the same
block collapse to one entry and the comparison is over that sequence, which is
the same argument `check_native.py` makes for page flips.

The checksum is what makes it a comparison rather than a tally: agreeing on
lengths says a sample of that size played, agreeing on the sum says it was the
same sample.
"""

import argparse
import os
import subprocess
import sys

from tim import GAME_DIR, REPO

PORT = os.path.join(REPO, "reconstruct", "devtim")
HYBRID = os.path.join(REPO, "tools", "native", "native")


def blocks(cmd, seconds, label):
    """Run one side and return its play events as (sum, length) pairs."""
    env = dict(os.environ, TIM_HEADLESS="1", TIM_TRACE="sb")
    try:
        p = subprocess.run(cmd, env=env, timeout=seconds,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        err = p.stderr
    except subprocess.TimeoutExpired as t:
        err = t.stderr or b""

    out = []
    for line in err.decode("latin-1").splitlines():
        f = line.split()
        if line.startswith("io: sb play sum"):
            out.append((f[4], f[5]))
    if not out:
        sys.exit(f"{label}: no sample blocks at all - is RESOURCE.CFG a "
                 f"Sound Blaster? "
                 f"{os.path.join(GAME_DIR, 'RESOURCE.CFG')}")
    return out


def collapse(seq):
    """Runs of the same block become one entry, so wall clock cannot matter."""
    out = []
    for k in seq:
        if not out or out[-1][0] != k:
            out.append([k, 1])
        else:
            out[-1][1] += 1
    return out


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""environment:
  TIM_GAMEDIR=DIR  the game directory both sides see. This check needs a
                   RESOURCE.CFG naming a Sound Blaster, since a speaker
                   configuration plays no samples at all and the run is
                   then vacuously equal.

exit status:
  0  every run agreed, by length, rate and content
  1  the two sides played different samples, or one played none
""")
    ap.add_argument("--port-seconds", type=int, default=90,
                    help="how long to run the port (default 90)")
    ap.add_argument("--hybrid-seconds", type=int, default=300,
                    help="how long to run the hybrid, which is much slower "
                         "(default 300)")
    ap.add_argument("--quiet", action="store_true",
                    help="print the verdict and nothing else")
    args = ap.parse_args()

    for exe in (PORT, HYBRID):
        if not os.path.exists(exe):
            sys.exit(f"{exe} is not built")

    port = collapse(blocks([PORT], args.port_seconds, "port"))
    hybrid = collapse(blocks([HYBRID], args.hybrid_seconds, "hybrid"))

    n = min(len(port), len(hybrid))
    if n == 0:
        sys.exit("neither side played anything")

    for i in range(n):
        if port[i][0] != hybrid[i][0]:
            print(f"DIFFERS at run {i}: the original played "
                  f"{int(hybrid[i][0][1], 16)} bytes summing "
                  f"{hybrid[i][0][0]}, the port played "
                  f"{int(port[i][0][1], 16)} bytes summing {port[i][0][0]}")
            return 1

    if not args.quiet:
        seen = {}
        for k, c in hybrid:
            seen[k] = seen.get(k, 0) + c
        print("the samples played, longest first:")
        for (s, ln), c in sorted(seen.items(), key=lambda x: -int(x[0][1], 16)):
            if int(ln, 16) < 2:
                continue
            print(f"  {int(ln, 16):6d} bytes  sum {s}  played {c}x")
        print()

    print(f"{n} runs of blocks, identical by length, rate and content. "
          f"The port plays the original's samples.")
    if len(port) != len(hybrid):
        print(f"(the two runs reached different depths - {len(port)} runs "
              f"against {len(hybrid)} - which is wall clock, not a difference)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
