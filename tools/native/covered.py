"""How much of one routine a run actually executed.

**A routine verified on one path is not verified.** `tools/verify.py` proves a
transcription by stopping at the routine's entry, letting the original body
run, and comparing what the two sides did to the machine. What it cannot say
is how much of the routine those calls went *through*: a branch nobody took is
a branch nobody checked, and it reads as a pass exactly like one that was.

So: run the hybrid with `TIM_COVER=<lo>:<hi>:<path>`, which marks every byte
the guest executed in that range, and hand the file here with the routine's
address. The routine's own instructions are found the way `iogap.py` finds
them - following jumps but not calls - and this says which of them were never
reached.

    TIM_COVER=f8c2:faf9:/tmp/c.txt ./tools/native/native ...
    uv run python tools/native/covered.py 0x0f8c2 /tmp/c.txt

**A snapshot hides the entry.** `TIM_RESTORE` puts the machine back in the
middle of whatever it was doing, so a routine the capture was taken *inside*
never executes its own prologue again and reads as uncovered from its first
instruction. `sub_0f8c2` measured 55.5% that way with `push bp` among the
misses, which is not a branch nobody took - it is a branch nobody could have
taken from that starting point. Measure from before the routine is entered, or
read the head of the miss list as the artefact it is.

100% means every instruction of the routine ran at least once. It does *not*
mean every path did - two branches that rejoin are both covered by one run
through each, but a condition that is always true covers its instructions and
proves nothing about the other side. Coverage is a floor on what was tested,
never a ceiling.

This file is the port's own tooling; it is not a transcription.
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, "tools")
import codemap
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)


def body(img, start):
    """Instruction addresses of one routine: jumps followed, calls not."""
    seen, queue = set(), [start]
    while queue:
        pc = queue.pop()
        while 0 <= pc < codemap.DGROUP and pc not in seen:
            ins = next(md.disasm(img[pc:pc + 16], pc), None)
            if ins is None:
                break
            seen.add(pc)
            m, ops = ins.mnemonic, ins.op_str
            nxt = pc + ins.size
            if m in ("call", "lcall"):
                pc = nxt
                continue
            if m in codemap.COND:
                if ops.startswith("0x"):
                    queue.append(int(ops, 16))
                pc = nxt
                continue
            if m == "jmp" and ops.startswith("0x"):
                pc = int(ops, 16)
                continue
            if m in codemap.STOP or m == "ljmp":
                break
            pc = nxt
    return seen


def main(argv):
    if len(argv) != 2:
        raise SystemExit(__doc__.strip().splitlines()[0]
                         + "\n\nusage: covered.py <routine> <cover-file>")
    at = int(argv[0], 0)
    img = codemap.image()
    mine = body(img, at)
    ran = {int(l, 16) for l in open(argv[1]) if l.strip()}

    hit = sorted(mine & ran)
    missed = sorted(mine - ran)
    pct = 100.0 * len(hit) / len(mine) if mine else 0.0

    print("0x%05x: %d instructions, %d executed - **%.1f%%**"
          % (at, len(mine), len(hit), pct))
    if not missed:
        print("every instruction ran at least once")
        return 0

    print("\nnever reached (%d):" % len(missed))
    for a in missed:
        ins = next(md.disasm(img[a:a + 16], a), None)
        print("   %05x  %-8s %s" % (a, ins.mnemonic if ins else "?",
                                    ins.op_str if ins else ""))
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
