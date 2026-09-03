"""What convention a routine uses, read from the routine.

Each entry in dispatch.c needs four facts - near or far, how many stack words,
what a `ret N` removes, and what it answers in - and every one of them has to
come off the disassembly. Guessing any of them desynchronises the guest's
stack, which surfaces as a crash somewhere else entirely; this project has the
scar tissue to prove it, and `dos_creat` sitting near-and-pascal among thirteen
far-and-cdecl siblings is the example in the file.

So this reads them:

    uv run python tools/native/conv.py 0x08f77 0x21094 ...

`ret` versus `retf` says near or far. The argument count comes from the port's
own prototype in tim.h, because the port is what will be called; the first
`[bp+N]` is printed beside it as a check, since a far routine's first argument
is at bp+6 and a near one's at bp+4, and a disagreement there means one of the
two readings is wrong and neither should be trusted.

**What it cannot see, and the entry is where all of it hides.** A routine that
begins `pop bx / push cs / push bx` is a near-to-far thunk: both its exits are
`retf` and its caller makes a near call, so the `retf` here answers "far" and
is wrong. A routine taking its arguments in registers has no `[bp+N]` to
disagree with. A routine answering in the flags - `cmp` then `retf` - gets a
RET_AX it does not want. And a range holding two routines shows two different
`ret N` values, which reads as one confused routine.

So this narrows the reading; it does not finish it. Look at the first three
instructions yourself, and at the port's prototype beside them.

This file is the port's own tooling; it is not a transcription.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def name_of(at):
    """The transcribed routine starting at this address, from syms.c."""
    src = open(os.path.join(ROOT, "tools", "native", "syms.c")).read()
    for a, n in re.findall(r'\{ (0x[0-9a-f]+), "(\w+)"', src):
        if int(a, 16) == at:
            return n
    return None


def prototype(name):
    """(return type, argument count) from the port's own header."""
    src = open(os.path.join(ROOT, "reconstruct", "tim.h")).read()
    m = re.search(r'^([a-z_0-9]+)\s+%s\(([^;]*?)\)\s*;' % re.escape(name),
                  src, re.M | re.S)
    if not m:
        return None, None
    args = m.group(2).strip()
    n = 0 if args in ("void", "") else len(args.split(","))
    return m.group(1), n


def convention(at, span=0x1200):
    out = subprocess.run(
        ["uv", "run", "python", os.path.join(ROOT, "tools", "disasm.py"),
         hex(at), "-e", hex(at + span)],
        capture_output=True, text=True, cwd=ROOT).stdout
    # Two shapes this must refuse rather than describe, because for both of
    # them the honest reading is the opposite of the obvious one and the
    # answer it would otherwise print corrupts the guest's stack.
    #
    # An `iret` before any `ret` means an interrupt handler: the machine owes
    # it six bytes back, and every return routines.def can express pops four
    # or two. A shim would leave the flags behind and every return after it
    # would read a word too high.
    #
    # `pop <r> / push cs / push <r>` at the entry is a near-to-far thunk. Both
    # its exits are `retf` and its caller makes a *near* call, so scanning for
    # the return answers "far" with complete confidence and is wrong.
    lines = out.split("\n")
    for line in lines:
        if re.match(r'^[0-9a-f]{5}\s+cf\s+iret\b', line):
            return "iret", 0, None
        if re.match(r'^[0-9a-f]{5}\s+\S+\s+retf?\b', line):
            break
    # The first three bytes, which are three *lines*: the listing prints one
    # instruction per line, and `pop bx` is one byte. Reading them from a
    # single line found nothing and the detector never fired.
    head = b""
    for line in lines:
        m2 = re.match(r'^[0-9a-f]{5}\s+([0-9a-f]+)\s', line)
        if not m2:
            continue          # a banner line, not an instruction
        head += bytes.fromhex(m2.group(1))
        if len(head) >= 3:
            break
    if (len(head) >= 3 and 0x58 <= head[0] <= 0x5f and head[1] == 0x0e
            and head[2] == head[0] - 8):
        return "thunk", 0, None

    m = re.search(r'^[0-9a-f]{5}\s+\S+\s+(retf?)\s*(0x[0-9a-f]+|\d+)?\s*$',
                  out, re.M)
    first = re.search(r'\[bp \+ (0x[0-9a-f]+|\d+)\]', out)
    kind = m.group(1) if m else None
    pops = int(m.group(2), 0) if m and m.group(2) else 0
    return kind, pops, (int(first.group(1), 0) if first else None)


def main(argv):
    if "-h" in argv or "--help" in argv:
        print(__doc__)
        print("usage: conv.py <routine> [<routine> ...]\n\n"
              "  <routine>  an image address (0x0f8c2) or a symbol name\n"
              "  -h/--help  this text\n\nReads no environment.")
        return 0
    if not argv:
        print(__doc__)
        return 2
    for a in argv:
        at = int(a, 0)
        name = name_of(at)
        kind, pops, firstarg = convention(at)
        rt, n = prototype(name) if name else (None, None)
        ret = ("RET_NONE" if rt == "void"
               else "RET_DXAX" if rt in ("uint32_t", "int32_t")
               else "RET_AX" if rt else "?")

        if not name:
            print("%#07x  not a transcribed routine's first address" % at)
            continue
        if n is None:
            print("%#07x  %-24s no prototype in tim.h - it may take its "
                  "arguments in registers, which this table does not marshal"
                  % (at, name))
            continue
        if kind == "iret":
            print("%#07x  %-24s ends `iret` - an interrupt handler. The machine"
                  " owes it six bytes; nothing here pops six." % (at, name))
            continue
        if kind == "thunk":
            print("%#07x  %-24s begins `pop <r> / push cs / push <r>` - a "
                  "near-to-far thunk. Its exits say `retf` and its caller "
                  "calls near; declare it near, or take the body after it."
                  % (at, name))
            continue
        if kind is None:
            print("%#07x  %-24s no `ret` within 0x400 bytes - widen the window"
                  % (at, name))
            continue

        # The lowest frame offset a routine touches must not be *below* where
        # its first argument can be: under bp+6 on a far frame is the return
        # segment, under bp+4 on a near one is the return offset itself.
        #
        # It may well be above. `stdio_fopen_into` reads only [bp+0xa] - its
        # fourth argument - and never looks at the first three, and an earlier
        # version of this check called that a mismatch and told the reader to
        # distrust a correct reading. A routine is not obliged to use what it
        # is given.
        want = 6 if kind == "retf" else 4
        note = ""
        if firstarg is not None and n and firstarg < want:
            note = ("   ** touches [bp+%#x], below the first argument at "
                    "[bp+%#x] for %s - the frame kind is wrong **"
                    % (firstarg, want, kind))
        elif firstarg is not None and n and (firstarg - want) % 2:
            note = ("   ** [bp+%#x] is not a word boundary from [bp+%#x] **"
                    % (firstarg, want))

        # A routines.def line, in the macro the reading calls for. The old
        # dispatch.c struct row this used to print for anything that was not
        # plain-far has not existed since the table became generated, so half
        # its answers were in a format nothing accepts.
        if kind == "retf" and not pops:
            print("    FAR_C (%#07x, %d, %-9s %s),%s"
                  % (at, n, ret + ",", name, note))
        elif kind == "retf":
            print("    FAR_P (%#07x, %d, %d, %-9s %s),%s"
                  % (at, n, pops, ret + ",", name, note))
        else:
            print("    NEAR_P(%#07x, %d, %d, %-9s %s),%s"
                  % (at, n, pops, ret + ",", name, note))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
