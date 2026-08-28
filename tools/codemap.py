"""Map the code before reading it.

Recursive descent from the entry point, following calls and branches. A linear
disassembly of 214 KB of 16-bit code desynchronises constantly - data sits
between routines - so the only addresses worth trusting are the ones control
flow actually reaches.

The binary is large model, so a far call carries its target segment as an
immediate and the entry point is 0000:0000. That makes a far target an image
offset outright: `lcall 0x1c25, 0x6233` is image 0x22483. No relocation
guesswork, and the *segment* of each routine is the translation unit it was
compiled in - which is how `--modules` recovers the original's source file
boundaries.

A `--run` pass adds what the running game actually reached, via Unicorn's basic
block hook: cheap enough to leave on for a full start-up, unlike a
per-instruction hook.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
from disasm import image, DGROUP

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

ENTRY = 0x0000

STOP = {"ret", "retf", "iret", "iretd", "jmp", "ljmp", "hlt"}
COND = {"je", "jne", "jz", "jnz", "js", "jns", "jo", "jno", "jb", "jae",
        "jbe", "ja", "jl", "jge", "jle", "jg", "jp", "jnp", "jcxz", "loop",
        "loope", "loopne"}


def far_target(op_str):
    """`0x1c25, 0x6233` -> image offset, since the image starts at segment 0."""
    try:
        seg, off = [int(p.strip(), 16) for p in op_str.split(",")]
    except ValueError:
        return None
    return seg * 16 + off


def walk(seeds, limit=None):
    """Recursive descent. Returns (instruction starts, call targets, edges)."""
    d = image()
    limit = limit if limit is not None else DGROUP
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    seen = set()
    calls = collections.Counter()
    callers = collections.defaultdict(set)
    queue = list(seeds)
    funcs = set(seeds)

    while queue:
        pc = queue.pop()
        if pc in seen or not (0 <= pc < limit):
            continue
        while 0 <= pc < limit and pc not in seen:
            ins = next(md.disasm(d[pc:pc + 16], pc), None)
            if ins is None:
                break
            seen.add(pc)
            m, ops = ins.mnemonic, ins.op_str
            nxt = pc + ins.size

            if m in ("call", "lcall"):
                t = far_target(ops) if m == "lcall" else (
                    int(ops, 16) if ops.startswith("0x") else None)
                if t is not None and 0 <= t < limit:
                    calls[t] += 1
                    callers[t].add(pc)
                    if t not in funcs:
                        funcs.add(t)
                        queue.append(t)
                pc = nxt
                continue

            if m in COND:
                if ops.startswith("0x"):
                    queue.append(int(ops, 16))
                pc = nxt
                continue

            if m == "jmp" and ops.startswith("0x"):
                pc = int(ops, 16)
                continue

            if m == "ljmp":
                t = far_target(ops)
                if t is not None:
                    queue.append(t)
                break

            if m in STOP:
                break
            pc = nxt
    return seen, calls, callers, funcs


def run_coverage(chunks, chunk_size=200_000):
    """Basic blocks the running game actually reaches, as image offsets."""
    from unicorn import UcError, UC_HOOK_BLOCK
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP

    tim.game_dir()
    m = tim.TimMachine(tim.UNPACKED_EXE)
    m.verbose = False
    m.max_insns = 10 ** 12
    base = m.load_seg * 16
    top = base + DGROUP
    blocks = set()
    overlay = set()

    def on_block(uc, address, size, ud):
        if base <= address < top:
            blocks.add(address - base)
        else:
            overlay.add(address)

    m.uc.hook_add(UC_HOOK_BLOCK, on_block)
    addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
    for _ in range(chunks):
        try:
            m.uc.emu_start(addr, 0, count=chunk_size)
        except UcError:
            break
        if m.finished:
            break
        addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
        m.service_keyboard()
        m.service_timer()
        addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
    return blocks, overlay


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--run", type=int, default=0, metavar="CHUNKS",
                    help="also run the game this many 200k-instruction chunks "
                         "and record the blocks it reaches")
    ap.add_argument("--modules", action="store_true",
                    help="group routines by code segment, which in a large "
                         "model binary is the original's translation unit")
    ap.add_argument("--json", default="", help="write the map here")
    args = ap.parse_args()

    seen, calls, callers, funcs = walk([ENTRY])
    d = image()
    print("recursive descent from %04x" % ENTRY)
    print("  instructions reached : %d" % len(seen))
    print("  call targets         : %d" % len(funcs))
    print("  code bytes covered   : %d of %d (%.1f%%)"
          % (len(seen) and sum(1 for _ in seen), DGROUP,
             100.0 * len(seen) / DGROUP))

    if args.modules:
        # A large-model module is a contiguous run of addresses with no other
        # module's code in the middle. Report the gaps between call targets so
        # the boundaries are visible.
        fl = sorted(funcs)
        print("\n%d routines, first 20 and the gaps between them:" % len(fl))
        for i, f in enumerate(fl[:20]):
            gap = (fl[i + 1] - f) if i + 1 < len(fl) else 0
            print("   %05x  callers=%-3d size~%d" % (f, calls.get(f, 0), gap))

    cov = None
    if args.run:
        blocks, overlay = run_coverage(args.run)
        reached = {f for f in funcs if f in blocks}
        print("\ndynamic pass (%d chunks):" % args.run)
        print("  blocks reached in image   : %d" % len(blocks))
        print("  blocks reached in overlays: %d (VM.OVL / SX.OVL)" % len(overlay))
        print("  call targets reached      : %d of %d (%.1f%%)"
              % (len(reached), len(funcs), 100.0 * len(reached) / max(1, len(funcs))))
        cov = dict(blocks=sorted(blocks), reached=sorted(reached))

    if args.json:
        out = dict(entry=ENTRY, dgroup=DGROUP,
                   funcs=sorted(funcs),
                   calls={hex(k): v for k, v in calls.items()},
                   coverage=cov)
        os.makedirs(os.path.dirname(args.json) or ".", exist_ok=True)
        json.dump(out, open(args.json, "w"), indent=1)
        print("\nwrote %s" % args.json)


if __name__ == "__main__":
    main()
