"""Which transcribed routines will trap if the guest runs them.

The hybrid traps on every `in`, every `out` and every A000 access, and that is
what finds a routine the port has a body for but the dispatcher does not know
about: the guest runs the original's copy, it touches hardware, and the run
stops with a backtrace. Working, and the design - but it finds them **one at a
time, by playing**, which means a routine nobody happens to exercise stays
hidden until someone drags the right part across the right screen.

`draw_bitmap_scaled` was found exactly that way, after 750 routines had been
transcribed and 221 dispatched. This exists so the next one does not have to
be.

Control flow is followed through jumps but **not** through calls, so a routine
is credited only with the hardware it touches *itself*. Without that every
caller inherits its callees' ports and `game_main` comes out as the busiest
routine in the program, which says nothing: the trap fires in the leaf that
executes the instruction, not in whoever called it.

**Port I/O only.** The A000 aperture is trapped too and is not detected here -
a write through ES:[di] cannot be told from any other far write without
knowing what ES holds - so a clean run of this is not a proof that nothing can
trap, only that nothing can trap *on a port*.

This file is the port's own tooling; it is not a transcription.
"""

import sys as _sys

if "-h" in _sys.argv[1:] or "--help" in _sys.argv[1:]:
    print(__doc__)
    print("usage: iogap.py\n\n"
          "  -h/--help  this text\n\n"
          "Takes no arguments and reads no environment. Answers which\n"
          "transcribed routines will trap if the guest runs them: every one\n"
          "whose own body does port I/O, and whether it is dispatched.")
    raise SystemExit(0)
import re, sys
sys.path.insert(0, "tools")
import codemap
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

img = codemap.image()
COND = codemap.COND
STOP = codemap.STOP
md = Cs(CS_ARCH_X86, CS_MODE_16)
PORTIO = {0xE4, 0xE5, 0xE6, 0xE7, 0xEC, 0xED, 0xEE, 0xEF}

def body(start, limit=None):
    """Instruction addresses of one routine: jumps followed, calls not."""
    limit = limit or codemap.DGROUP
    seen, queue = set(), [start]
    while queue:
        pc = queue.pop()
        while 0 <= pc < limit and pc not in seen:
            ins = next(md.disasm(img[pc:pc + 16], pc), None)
            if ins is None:
                break
            seen.add(pc)
            m, ops = ins.mnemonic, ins.op_str
            nxt = pc + ins.size
            if m in ("call", "lcall"):
                pc = nxt; continue
            if m in COND:
                if ops.startswith("0x"):
                    queue.append(int(ops, 16))
                pc = nxt; continue
            if m == "jmp" and ops.startswith("0x"):
                pc = int(ops, 16); continue
            if m in STOP or m == "ljmp":
                break
            pc = nxt
    return seen

have = {}
for m in re.finditer(r'\{ (0x[0-9a-f]+), "([^"]+)", (\d) \}',
                     open("tools/native/syms.c").read()):
    have[int(m.group(1), 16)] = (m.group(2), int(m.group(3)))
disp = set()
for m in re.finditer(r"^\s*(?:FAR_C|FAR_P|FAR_R|NEAR_C|NEAR_P|REG_N|REG_F)\s*\(\s*(0x[0-9a-f]+)",
                     open("tools/native/routines.def").read(), re.M):
    disp.add(int(m.group(1), 16))

rows = []
for at, (name, stub) in sorted(have.items()):
    if stub:
        continue
    n = sum(1 for pc in body(at) if pc < len(img) and img[pc] in PORTIO)
    if n:
        rows.append((at, name, n, at in disp))

undisp = [r for r in rows if not r[3]]
print("routines whose own body does port I/O: %d" % len(rows))
print("   of those, dispatched:     %d" % (len(rows) - len(undisp)))
print("   NOT dispatched (latent traps): %d\n" % len(undisp))
for at, name, n, _ in undisp:
    print("   0x%05x  %-28s %d" % (at, name, n))
