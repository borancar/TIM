"""What one routine calls that the port does not have yet.

Before transcribing a routine it is worth knowing what it will drag in with
it, and the obvious way to find out - walk the call graph and stop at what is
already written - answers a *different* question than the one that matters.
`codemap.walk` follows calls through routines the port already has, so it
reports everything reachable from those too: `region_click_bin` came out as 44
missing routines that way, of which 42 were reachable only through code the
port had answered for years and two were the actual work.

So this reports a routine's **direct** calls only, following jumps but not
calls, exactly as `covered.py` and `iogap.py` bound a routine's own body. Run
it again on whatever it names to go a level deeper; the tree you get that way
is the tree you have to write, not the tree the program can reach.

    uv run python tools/native/needs.py 0x0f8c2 [0x... ...]

A stub counts as missing: it has an address and a name and no body.

This file is the port's own tooling; it is not a transcription.
"""
import sys, os, re
sys.path.insert(0, "tools")
import codemap
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)


def symbols():
    have = {}
    path = os.path.join(os.path.dirname(__file__), "syms.c")
    for m in re.finditer(r'\{ (0x[0-9a-f]+), "([^"]+)", (\d) \}', open(path).read()):
        have[int(m.group(1), 16)] = (m.group(2), int(m.group(3)))
    return have


def body(img, start):
    """(instructions, direct call targets, indirect branch count).

    Jumps are followed, calls are not. **An indirect branch is counted, not
    followed**: `jmp word ptr cs:[bx+0x1cfe]` is a jump table whose length is
    not in the instruction, so the arms are invisible here. That is not a
    detail - `0x0fc0e` reported one missing dependency and has five, because
    all but one are reached through exactly such a table. A routine with any
    indirect branch has its cost reported as a **lower bound** and says so.
    """
    seen, queue, calls, indirect = set(), [start], [], 0
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
                t = codemap.far_target(ops) if m == "lcall" else (
                    int(ops, 16) if ops.startswith("0x") else None)
                # **A target outside the image is not a target.** Walking off
                # the end of a routine into data disassembles bytes that were
                # never instructions, and a `call` invented that way lands
                # anywhere - 0xffffd4ff and friends, which were then reported
                # as routines to transcribe. Bounds-check before believing it.
                if t is not None and 0 <= t < codemap.DGROUP:
                    calls.append(t)
                elif t is None:
                    indirect += 1
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
            if m == "jmp":
                indirect += 1
                break
            if m in codemap.STOP or m == "ljmp":
                break
            pc = nxt
    return seen, calls, indirect


def main(argv):
    recurse = "--tree" in argv
    argv = [a for a in argv if a != "--tree"]
    if not argv:
        raise SystemExit("usage: needs.py [--tree] <routine> [<routine> ...]")
    img = codemap.image()
    have = symbols()

    seen_r, order, queue = set(), [], [int(a, 0) for a in argv]
    while queue:
        at = queue.pop(0)
        if at in seen_r:
            continue
        seen_r.add(at)
        insns, calls, indirect = body(img, at)
        uniq = list(dict.fromkeys(calls))
        miss = [c for c in uniq if c not in have or have[c][1] == 1]
        order.append((at, len(insns), uniq, miss, indirect))
        if recurse:
            queue.extend(miss)

    total = 0
    bound = 0
    for at, n, uniq, miss, indirect in order:
        name = have.get(at, ("(no symbol)", 0))[0]
        total += n
        bound += indirect
        print("0x%05x  %-28s %3d instructions, %2d calls, %d missing%s"
              % (at, name, n, len(uniq), len(miss),
                 "   [%d indirect - LOWER BOUND]" % indirect if indirect else ""))
        for c in miss:
            k = have.get(c)
            print("    0x%05x  %s" % (c, (k[0] + "  (STUB)") if k else ""))

    if recurse:
        print("\n%d routines, %d instructions%s"
              % (len(order), total,
                 "  -- and %d indirect branches whose arms are not counted, so"
                 " this is a floor" % bound if bound else ""))
    else:
        todo = [c for _a, _n, _u, m, _i in order for c in m]
        if todo:
            print("\nnext: uv run python tools/native/needs.py %s"
                  % " ".join("0x%05x" % c for c in dict.fromkeys(todo)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
