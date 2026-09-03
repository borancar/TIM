"""What to transcribe next, in an order that keeps the stubs down.

Transcribing bottom-up matters: a routine whose callees are already transcribed
can be verified end to end, while one full of stubs can only be checked on the
paths that avoid them. So this orders the routines a screen actually reaches by
how many *untranscribed* callees they have, then by size.

"Transcribed" is read from the port's own sources - the same addresses
tests/provenance.py checks - so the queue cannot drift from what exists.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import bisect
import glob
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
from codemap import walk, ENTRY, far_target, kind_hook_seeds
from disasm import DGROUP, image

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

# A transcribed routine's address, as it appears in the comment directly above
# it. Two forms: the plain image offset on its own, and the segment-relative
# form the segment-172c routines carry - `172c:49a1, image 0x1bc61` - because
# the offset is what the DGROUP hook tables hold and both are worth naming.
#
# A one-line `/* 0x002a1 */` counts too. The six do-nothing hooks at the top of
# segment 0000 are written that way - there is nothing to say about them beyond
# the address - and requiring a block comment read all six as untranscribed.
#
# The address may be followed by a dash and a phrase naming what the routine is
# - `172c:2f3e, image 0x1a1fe - kind 6's step` - which is how most of segment
# 172c is written. Requiring the line to end at the address read every one of
# those as untranscribed, and the screen's coverage came out several routines
# short of the truth. Nothing else may follow it: a bare `0x` anywhere in the
# prose is not a provenance line, and matching one would count a routine that
# does not exist.
ADDR = re.compile(
    r"^\s*(?:\*|/\*)\s*(?:[0-9a-f]{4}:[0-9a-f]{4},\s*image\s*)?"
    r"(0x[0-9a-f]{4,5})(?:[.,]|\s+-\s+\S[^*]*)?\s*(?:\*/)?\s*$", re.M)


def transcribed():
    """Image offsets the port says it has transcribed."""
    out = set()
    for path in glob.glob(os.path.join(tim.REPO, "reconstruct", "**", "*.c"),
                          recursive=True):
        txt = open(path).read()
        for m in ADDR.finditer(txt):
            out.add(int(m.group(1), 16))
    return out


# Borland runtime helpers, each read before being classified - see
# docs/runtime.md. They are deliberate non-goals: a 32-bit shift is `<<` in the
# port, so there is nothing to call. Nothing goes in here unguessed.
RUNTIME = {
    0x0BD90, 0x0BD93, 0x0BD97, 0x0BD9F,   # long comparisons
    0x0BE3E, 0x0BE41,                     # long shift left
    0x0C16E,                              # long multiply
    0x0C7C4,                              # stack overflow check
    0x0BE5F, 0x0BE62,                     # long shift right, and its far entry
    0x0BD0D,                              # far pointer normalise and compare
    0x0D543,                              # memset
    0x00274,                              # write to stderr
    0x0C7E6,                              # heap extension (sbrk)
    0x0DD55,                              # case-insensitive string compare
    0x0CA39,                              # malloc
    0x0BFCD, 0x0C006,                     # errno / _doserrno mapping
    0x0CD3D,                              # chmod / get file attributes
    0x0C0C3,                              # lseek
    0x0C185,                              # read
}


# The C runtime occupies the **top of segment 0000**, from 0x0bd00 to the end
# of the segment at 0x0dff0. This is a claim about the module layout, not 85
# individual identifications, and it is kept separate from RUNTIME above for
# exactly that reason. The evidence:
#
#   - 23 routines in that range have been read one by one and every one is
#     Borland's: stdio, malloc, long arithmetic, errno, file I/O.
#   - Below the line, 19 routines have been read and all are the game's; the
#     only runtime routine below it is the stderr write at 0x00274, which
#     belongs to the start-up module at the very bottom of the segment.
#   - `find_free_slot_4bc4` at 0x0d0a3 was transcribed as game code and was
#     wrong - it scans Borland's array of 16-byte FILE structures for one whose
#     signed `fd` at +4 is negative. It was the only apparent exception and it
#     turned out not to be one.
#   - The linker lays each module's contribution down whole and in link order,
#     and the runtime links last, so a contiguous run at one end is what a
#     runtime cluster looks like.
#
# Anything here that later proves to be the game's own is a retraction to
# record, not a surprise to absorb quietly.
RUNTIME_TOP_OF_SEG0 = 0x0BD00
SEG0_END = 0x0DFF0


def in_runtime_block(f):
    return RUNTIME_TOP_OF_SEG0 <= f < SEG0_END


# Borland's large model gives near runtime routines a **far wrapper**: push the
# arguments straight back, call the near one, clean up, return. They sit just
# below the runtime block rather than inside it, and they are the game's code
# only in the sense that the compiler emitted them into its segment.
#
# This is recognised structurally rather than by address: a routine qualifies
# only if its whole body is argument forwarding around exactly one call, and
# that call lands in the runtime block. Nothing that does any work of its own
# can match.
FORWARD_OK = {"push", "nop", "call", "lcall", "add", "inc", "pop", "mov",
              "ret", "retf", "sub"}


def is_runtime_forwarder(lo, hi):
    d = image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    calls, pc, n = [], lo, 0
    while pc < hi:
        ins = next(md.disasm(d[pc:pc + 16], pc), None)
        if ins is None:
            return False
        n += 1
        if n > 14:
            return False
        m = ins.mnemonic
        if m not in FORWARD_OK:
            return False
        if m in ("call", "lcall"):
            t = far_target(ins.op_str) if m == "lcall" else (
                int(ins.op_str, 16) if ins.op_str.startswith("0x") else None)
            if t is None:
                return False
            calls.append(t)
        if m == "mov" and "[" in ins.op_str and "bp" not in ins.op_str:
            return False
        pc += ins.size
        if m in ("ret", "retf"):
            break
    return len(calls) == 1 and in_runtime_block(calls[0])


# Segment 2619, the sound module: on the screens' execution path but not on
# the drawing path - every A000 write of nine frames was attributed to VM.OVL,
# reached from segments 0000 and 1c25. Deferred against the goal of matching
# the two screens; see STATUS.md.
SOUND_SEG_LO, SOUND_SEG_HI = 0x26190, 0x2A040


def in_sound_module(f):
    return SOUND_SEG_LO <= f < SOUND_SEG_HI


def is_thunk(f):
    """Four bytes of `ljmp [imm16]`: a call into the video driver."""
    d = image()
    return d[f] == 0xFF and d[f + 1] == 0x2E


def callees(lo, hi):
    """Direct call targets inside [lo, hi)."""
    d = image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    out = set()
    pc = lo
    while pc < hi:
        ins = next(md.disasm(d[pc:pc + 16], pc), None)
        if ins is None:
            pc += 1
            continue
        if ins.mnemonic == "lcall" and "," in ins.op_str:
            t = far_target(ins.op_str)
            if t is not None and t < DGROUP:
                out.add(t)
        elif ins.mnemonic == "call" and ins.op_str.startswith("0x"):
            out.add(int(ins.op_str, 16))
        pc += ins.size
    return out


def data_bytes(fl, ends):
    """Bytes inside a routine's span that are data and not code.

    Keyed by the routine the span method attributes them to. Only the one
    region so far, and it is here rather than as a subtracted constant so the
    figure survives the routines around it moving.
    """
    out = {}

    # 0x2a941 is `arctan_lookup`. Everything from the end of its body to the
    # next routine is its table: the sine, cosine and arctangent lookups the
    # five routines in segment 2a04 index. Verified by reading it - the first
    # twenty words are 216, 218, 219, 220, 221, 222, 223, 224, 226 and on up,
    # which no instruction stream looks like.
    for f in fl:
        if f != 0x2a941:
            continue
        body = routine_body_end(f)
        if body is not None and ends[f] > body:
            out[f] = ends[f] - body
    return out


def routine_body_end(start):
    """One past the last instruction of a routine, following its jumps."""
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(
            os.path.abspath(__file__)), "native"))
        from covered import body, md
        import codemap as cm
    except Exception:
        return None

    img = cm.image()
    last = max(body(img, start))
    ins = next(md.disasm(img[last:last + 16], last), None)
    return last + (ins.size if ins else 0)


def walked_bytes(done):
    """Instruction bytes inside the transcribed routines, jumps followed.

    The honest half of the byte figure: no gap between two routines is credited
    to either of them, so nothing untranscribed is counted by accident.
    """
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(
            os.path.abspath(__file__)), "native"))
        from covered import body, md
        import codemap as cm
    except Exception:
        return None

    img = cm.image()
    seen = set()
    for f in done:
        if f >= DGROUP:
            continue
        for pc in body(img, f):
            ins = next(md.disasm(img[pc:pc + 16], pc), None)
            if ins:
                seen.update(range(pc, pc + ins.size))
    return len(seen)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--used", default="out/reached_title.json",
                    help="output of tools/reached.py --json")
    ap.add_argument("-n", type=int, default=25)
    ap.add_argument("--sound", action="store_true",
                    help="include segment 2619, the sound module")
    ap.add_argument("--no-runtime", action="store_true",
                    help="only routines that never call into the C runtime. "
                         "100 of the 139 remaining are like this, so the "
                         "unsettled question of what the port should do about "
                         "malloc and free need not block anything yet")
    args = ap.parse_args()

    seen, calls, callers, funcs = walk([ENTRY] + kind_hook_seeds())
    fl = sorted(funcs)
    ends = {f: (fl[i + 1] if i + 1 < len(fl) else DGROUP)
            for i, f in enumerate(fl)}

    used = set(json.load(open(args.used))["used"]) if os.path.exists(args.used) \
        else set(fl)
    done = transcribed()

    rows = []
    skipped = {"transcribed": 0, "runtime": 0, "runtime_block": 0,
               "forwarder": 0, "sound": 0, "thunk": 0}
    for f in sorted(used):
        if f in done:
            skipped["transcribed"] += 1
            continue
        if f in RUNTIME:
            skipped["runtime"] += 1
            continue
        if in_runtime_block(f):
            skipped["runtime_block"] += 1
            continue
        if is_runtime_forwarder(f, ends[f]):
            skipped["forwarder"] += 1
            continue
        if in_sound_module(f) and f not in done and not args.sound:
            skipped["sound"] += 1
            continue
        if is_thunk(f):
            skipped["thunk"] += 1
            continue
        cs = {c for c in callees(f, ends[f]) if c in funcs and c != f}
        missing = sorted(c for c in cs
                         if c not in done and c not in RUNTIME
                         and not in_runtime_block(c) and not is_thunk(c)
                         and not is_runtime_forwarder(c, ends.get(c, c + 1)))
        if args.no_runtime:
            rt = [c for c in cs if c in RUNTIME or in_runtime_block(c)
                  or is_runtime_forwarder(c, ends.get(c, c + 1))]
            if rt:
                continue
        rows.append((len(missing), ends[f] - f, f, calls.get(f, 0), missing))

    rows.sort()
    print("%d routines reached by the screen: %d transcribed, %d runtime "
          "(read), %d runtime (top of segment 0000), "
          "%d far wrappers on it, %d in the sound module (skipped), "
          "%d dispatch thunks, %d to go"
          % (len(used), skipped["transcribed"], skipped["runtime"],
             skipped["runtime_block"], skipped["forwarder"],
             skipped["sound"], skipped["thunk"], len(rows)))
    # How much of the original is actually reconstructed, in **bytes of its own
    # code** rather than in routines. A routine count flatters the port badly:
    # the routines transcribed first are the small leaves, and one 2,283-byte
    # table counts the same as a two-line thunk. Bytes are what there is to do.
    # **A routine's size here is the gap to the next one, and that gap is not
    # always code.** Segment 2a04 ends with the trigonometry tables: between
    # `arctan_lookup` and `atan2_long` there are about ten thousand bytes that
    # read `216, 218, 219, 220, 221, ...` - a smooth monotonic run, not
    # instructions. The span method hands all of them to `arctan_lookup`, and
    # because that routine *is* transcribed they counted as transcribed code in
    # both halves of the figure: the numerator and the denominator were each
    # about ten kilobytes too big, which flattered the percentage.
    #
    # `data_span` measures it from the two routines that bracket it rather than
    # from a constant, so it stays right if either moves. Anything else in the
    # image that turns out to be data belongs in the same list, with the same
    # kind of evidence beside it - a table nobody has looked at is not a table.
    tables = data_bytes(fl, ends)

    def size(f):
        return ends[f] - f - tables.get(f, 0)

    code_bytes = sum(size(f) for f in fl)
    done_bytes = sum(size(f) for f in fl if f in done)
    used_bytes = sum(size(f) for f in fl if f in used)
    used_done = sum(size(f) for f in fl if f in used and f in done)
    # The driver overlay is **not** in the code map above: that walks the main
    # image from its entry point, and VM.OVL is loaded separately. Measuring it
    # the same way would need a second recursive descent seeded from its vector
    # table; until then it is counted here from the routine starts that *are*
    # known - the ones docs/video-driver.md read off the running machine, plus
    # every address the port has transcribed - with each routine running to the
    # next known start. That is a **lower bound**: a gap between two known
    # starts that holds two routines counts as one.
    drv_known = set()
    doc = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "docs", "video-driver.md")
    if os.path.exists(doc):
        for m in re.finditer(r"VGA:([0-9a-f]{4})", open(doc).read()):
            drv_known.add(int(m.group(1), 16))
    drv_done = set()
    for f in glob.glob(os.path.join(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__))), "reconstruct", "**", "*.c"),
            recursive=True):
        for m in re.finditer(r"^ \* VM\.OVL VGA:0x([0-9a-f]{4})\s*$",
                             open(f).read(), re.M):
            drv_done.add(int(m.group(1), 16))
    drv_known |= drv_done
    dl = sorted(drv_known)
    DRV_END = 0x2B10                      # what verify.py treats as the driver
    dends = {a2: (dl[i + 1] if i + 1 < len(dl) else DRV_END)
             for i, a2 in enumerate(dl)}
    drv_bytes = sum(dends[a2] - a2 for a2 in dl if a2 < DRV_END)
    drv_ours = sum(dends[a2] - a2 for a2 in drv_done if a2 < DRV_END)
    print("driver:   %d of %d routines, at least %d of %d bytes of VM.OVL (%.1f%%)"
          % (len(drv_done), len(dl), drv_ours, drv_bytes,
             100.0 * drv_ours / max(1, drv_bytes)))
    # Segment 172c is not in the code map either, and for the same reason the
    # driver is not: **nothing calls into it directly**. Every one of its
    # routines is reached through a relocated far pointer in a table, and
    # recursive descent cannot follow one. So the parts work is invisible to the
    # figure below in exactly the way VM.OVL was, and is counted here instead.
    #
    # A setup counts as transcribed when its offset appears in one of the
    # tables in parts.c or in one of the `off ==` cases beside them. Both
    # forms are counted because both are how a setup gets reconstructed here -
    # counting only the first said 11 of 39 when the answer was all of them.
    src = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "reconstruct", "src", "parts.c")
    if os.path.exists(src):
        text = open(src).read()
        # **Not `done`.** That name already holds every transcribed routine
        # address, and reusing it here quietly replaced the set that the byte
        # figures below are measured from - which read 4.3% instead of 79%
        # until the shadowing was noticed.
        setups = set()
        for m in re.finditer(r"\{ (0x[0-9a-f]{4}),", text):
            setups.add(int(m.group(1), 16))
        for m in re.finditer(r"off == (0x[0-9a-f]{3,4})", text):
            setups.add(int(m.group(1), 16))
        want = {int(m.group(1), 16) for m in re.finditer(
            r"\{ 0x[0-9a-f]+, 0x[0-9a-f]+, 0x[0-9a-f]+, 0x[0-9a-f]+, (0x[0-9a-f]+) \}",
            open(os.path.join(os.path.dirname(src), "machine_draw.c")).read())}
        if want:
            print("parts:    %d of the %d part setups segment 172c holds"
                  % (len(setups & want), len(want)))
        else:
            # The table this counted moved out of machine_draw.c, so the pattern
            # matches nothing and the line used to print "0 of the 0" - a
            # measurement that had stopped measuring and went on being printed
            # as though it had. Say so instead.
            print("parts:    not counted - the table this reads has moved out "
                  "of machine_draw.c and the pattern no longer matches")

    print("bytes:    %d of %d of all reachable code transcribed (%.1f%%); "
          "%d of %d that this screen reaches (%.1f%%)"
          % (done_bytes, code_bytes, 100.0 * done_bytes / max(1, code_bytes),
             used_done, used_bytes, 100.0 * used_done / max(1, used_bytes)))

    # **And the same question asked without the span method's bias.**
    #
    # Everything above sizes a routine as the gap to the next one, so a
    # transcribed routine is credited with any untranscribed routine that
    # follows it without a symbol of its own - and the figure comes out far too
    # kind. Measured as spans the port reads 93%; measured as the instruction
    # bytes each transcribed routine actually contains, 79%.
    #
    # Both are printed because neither is the whole answer. The span figure is
    # an upper bound and the body figure a lower one: the body walk follows
    # jumps but not calls, so a routine reached only through a table it does
    # not itself branch to is missed, and the tables and jump islands still
    # sitting in the denominator are not code anybody has to write.
    walked = walked_bytes(done)
    if walked is not None:
        print("bodies:   %d of %d as real instruction bytes (%.1f%%) - the "
              "span figure above is an upper bound"
              % (walked, code_bytes, 100.0 * walked / max(1, code_bytes)))
    print("\n%-8s %-6s %-8s %s" % ("addr", "bytes", "callers", "untranscribed callees"))
    for nmiss, size, f, ncall, missing in rows[:args.n]:
        shown = " ".join("%05x" % c for c in missing[:6])
        more = "" if len(missing) <= 6 else " +%d" % (len(missing) - 6)
        print("%05x    %-6d %-8d %s%s" % (f, size, ncall, shown, more))


if __name__ == "__main__":
    main()
