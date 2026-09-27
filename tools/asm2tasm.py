"""A draft TASM source for one assembly module, read out of the image.

The game's hand-written modules have to be source that TASM assembles back to
the image's bytes (CLAUDE.md, "An assembly module is a `.c` file too"). Typing
that source instruction by instruction from a disassembly is where the errors
come from - a short jump written as a near one, a far call to the wrong
routine, a displacement that was a variable written as a number. This reads the
image instead and prints the block's contents, and **the judge then says
whether it is right**: nothing here is trusted, it is a draft to be read.

What it knows, and from where:

- **Routine starts** are the port's own provenance for the range (the same
  `addresses()` the judge uses) and any `--at` given on the command line. A
  routine is `far` if its first return is `retf`.
- **Branches** get a label at their target, and the form the image used:
  `jmp short` for `EB`, `jmp near ptr` for `E9`, so TASM's one pass cannot
  choose differently.
- **A far call** is `call far ptr _name`, named from the address it lands on,
  with an `extrn` for anything outside the module. The image says a `9A` is
  one: its segment word is in the relocation table.
- **A segment immediate** - a word the relocation table names - is written as
  the segment it is (`DGROUP` for DGROUP's paragraph).
- **A direct DS operand** is written as the DGROUP object the port places
  there plus the offset into it, `DGROUP:_VMDS+12h`, from the placements
  (`DGROUP_AT`/`_BSS`/`_WAS`) in the port's sources; an offset no object
  covers stays a number and says so in a comment.

What it cannot know: whether an **immediate** was an address. `mov ax,
2d4ah` and `mov ax, offset find_name` assemble to the same three bytes, and
the draft writes the number. Reading the draft is where that is settled.

    uv run python tools/asm2tasm.py 0x08f27 0x08fc3 [--segment _TEXT]
"""
import argparse
import bisect
import glob
import os
import re
import struct
import sys

import capstone

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import judge                                            # noqa: E402
import cparse                                           # noqa: E402

DGROUP_PARA = judge.IMG_DGROUP // 16


def relocations():
    """Image offsets of every relocated word."""
    exe = open(judge.UNPACKED, "rb").read()
    nrel = struct.unpack_from("<H", exe, 6)[0]
    ro = struct.unpack_from("<H", exe, 0x18)[0]
    return {seg * 16 + off for off, seg in
            (struct.unpack_from("<HH", exe, ro + 4 * i) for i in range(nrel))}


def placements():
    """(address, name) for every DGROUP object the port places, sorted."""
    out = []
    for path in judge.port_sources():
        for _struct, name, addr in cparse.placements(path):
            out.append((addr, name))
    return sorted(set(out))


def data_name(off, placed, addrs):
    """`_NAME+N` for a DGROUP offset, from the object placed at or below it."""
    i = bisect.bisect_right(addrs, off) - 1
    if i < 0:
        return None
    base, name = placed[i]
    # inside that object: below the next one, and not far past the last
    end = addrs[i + 1] if i + 1 < len(addrs) else base + 0x200
    if off >= end:
        return None
    return "_%s+%s" % (name, hexnum(off - base)) if off != base else "_" + name


class Byte:
    """A byte that is data, standing where an instruction would."""
    mnemonic = "db"
    op_str = ""

    def __init__(self, address, value):
        self.address, self.bytes, self.size = address, bytes([value]), 1

    def group(self, g):
        return False


def is_branch(ins):
    """A relative transfer: capstone leaves `loop` and `jcxz` out of its
    jump group."""
    return (ins.group(capstone.CS_GRP_JUMP) or ins.mnemonic == "call"
            or ins.mnemonic in ("loop", "loope", "loopne", "jcxz"))


def hexnum(v):
    s = "%xh" % v
    return "0" + s if s[0] in "abcdef" else s


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("lo", type=lambda s: int(s, 0))
    ap.add_argument("hi", type=lambda s: int(s, 0))
    ap.add_argument("--segment", default="_TEXT")
    ap.add_argument("--data", nargs=2, type=lambda s: int(s, 0),
                    metavar=("LO", "HI"),
                    help="the module's own _DATA, emitted from the image with a "
                         "label at every offset the code names")
    ap.add_argument("--bss", nargs=2, type=lambda s: int(s, 0),
                    metavar=("LO", "HI"),
                    help="the module's own _BSS, emitted as reserved bytes with "
                         "a label at every offset the code names")
    ap.add_argument("--offsets", action="store_true",
                    help="write an index register loaded with a value inside a "
                         "placed object as `offset` - right for polyclip.c's "
                         "VMDS copies, wrong for LZHUF's tree constants, so "
                         "asked for and then read")
    ap.add_argument("--code-lead", action="store_true",
                    help="what precedes the first routine is code (a routine's "
                         "own head, reached by a branch), not data")
    ap.add_argument("--ds-is-cs", action="store_true",
                    help="the module's routines run with DS on their own code "
                         "segment: a direct operand is the code segment's data")
    ap.add_argument("--table", action="append", default=[],
                    help="LO:HI of a table inside the range: data, not code, "
                         "even where the port names its address")
    ap.add_argument("--at", action="append", default=[],
                    help="name=0xADDR for a routine the port has no name for")
    a = ap.parse_args(argv)

    img = open(judge.IMAGE, "rb").read()
    rel = relocations()
    known = judge.runtime_names()
    known.update(judge.addresses(judge.port_sources()))
    for spec in a.at:
        n, v = spec.split("=")
        known[n] = int(v, 0)
    by_addr = {}
    for n, v in known.items():
        by_addr.setdefault(v, n)
    placed = placements()
    addrs = [p[0] for p in placed]
    fr = judge.frames()

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True
    tables = [tuple(int(x, 0) for x in t.split(":")) for t in a.table]
    in_table = lambda x: any(lo <= x < hi for lo, hi in tables)
    starts = sorted(v for v in by_addr if a.lo <= v < a.hi and not in_table(v))
    # **Each routine decoded from its own start**, so data before one - a
    # module's state in its own code segment, as the sound driver keeps it -
    # cannot pull the decoding out of step. What precedes the first routine
    # is data.
    insns = []
    pre = []
    first = starts[0] if starts else a.hi
    if a.code_lead:
        insns.extend(md.disasm(img[a.lo:first], a.lo))
    else:
        for k in range(a.lo, first):
            pre.append(k)
    edges = sorted(set(starts + [x for t in tables for x in t if a.lo <= x <= a.hi]
                       + [a.hi]))
    for k, st in enumerate(edges[:-1]):
        if st < first and not a.code_lead:
            continue
        if in_table(st):
            # a table is bytes; one pseudo-instruction per byte
            for x in range(st, edges[k + 1]):
                insns.append(Byte(x, img[x]))
        else:
            got = list(md.disasm(img[st:edges[k + 1]], st))
            insns.extend(got)
            end = got[-1].address + got[-1].size if got else st
            for x in range(end, edges[k + 1]):
                insns.append(Byte(x, img[x]))

    # labels: every branch target inside the range that is not a routine
    labels = {}
    for ins in insns:
        if is_branch(ins):
            op = ins.op_str
            if re.fullmatch(r"0x[0-9a-f]+", op) and ins.bytes[0] != 0x9A:
                t = int(op, 16)
                if a.lo <= t < a.hi and t not in starts:
                    labels[t] = "L%05x" % t

    # **The code segment named as data**: a saved vector after a routine,
    # or - self-modifying code - the immediate byte of an instruction the
    # routine patches. Every `cs:` operand's target inside the range.
    cs_targets = set()
    for ins in insns:
        for mc in re.finditer(r"(cs:|(?<![a-z]:))\[(?:\w\w \+ )?(0x[0-9a-f]+)\]", ins.op_str):
            if mc.group(1) != "cs:" and not a.ds_is_cs:
                continue
            if re.search(r"\b(es|ss):\[", ins.op_str) and mc.group(1) != "cs:":
                continue
            t = judge.frame_of(ins.address, fr) + int(mc.group(2), 16)
            if a.lo <= t < a.hi:
                cs_targets.add(t)
    externs = {}
    own = set()
    a.own = own
    a.data_externs = set()
    out = []
    publics = [by_addr[s] for s in starts]
    out.append("%s segment byte public 'CODE'" % a.segment)
    out.append("assume cs:%s, ds:%s" % (a.segment, a.segment if a.ds_is_cs else "DGROUP"))
    for i in range(0, len(publics), 4):
        out.append("public " + ", ".join("_" + p for p in publics[i:i + 4]))
    far_procs = {}
    for k, st in enumerate(starts):
        end = starts[k + 1] if k + 1 < len(starts) else a.hi
        for j in md.disasm(img[st:end], st):
            if j.mnemonic in ("retf", "ret", "iret"):
                far_procs[by_addr[st]] = j.mnemonic == "retf"
                break
    body = []
    # the leading data, a label at each offset the code names
    run = []
    for k in pre:
        if k in cs_targets and run:
            body.append("        db " + ", ".join(run)); run = []
        if k in cs_targets:
            body.append("c_%05x label byte" % k)
        run.append(hexnum(img[k]))
        if len(run) == 16:
            body.append("        db " + ", ".join(run)); run = []
    if run:
        body.append("        db " + ", ".join(run))
    open_proc = None
    dead = False
    last_data_end = None
    for ins in insns:
        at = ins.address
        if at in labels or at in starts:
            dead = False
        if dead or isinstance(ins, Byte) or in_table(at):
            # **Bytes no path reaches**: the code segment's own data - a
            # saved vector, a table - between a routine's last transfer and
            # the next label. Written as bytes, sixteen to a line, with a
            # `c_` label wherever the code names one or a run begins.
            vals = [hexnum(x) for x in ins.bytes]
            if (body and body[-1].startswith("        db ") and not body[-1].endswith("*/")
                    and body[-1].count(",") < 15 and at not in cs_targets
                    and last_data_end == at):
                body[-1] += ", " + ", ".join(vals)
            else:
                if body and body[-1] == "c_%05x label byte" % at:
                    body.pop()
                body.append("c_%05x label byte" % at)
                body.append("        db " + ", ".join(vals))
            last_data_end = at + ins.size
            continue
        if at in by_addr and at in starts:
            if open_proc:
                body.append("_%s endp" % open_proc)
            nxt = [s for s in starts if s > at]
            end = nxt[0] if nxt else a.hi
            kind = "near"
            for j in md.disasm(img[at:end], at):
                if j.mnemonic in ("retf", "ret"):
                    kind = "far" if j.mnemonic == "retf" else "near"
                    break
            open_proc = by_addr[at]
            body.append("")
            body.append("/* 0x%05x */" % at)
            body.append("_%s proc %s" % (open_proc, kind))
        if at in labels:
            body.append("%s:" % labels[at])
        if at in cs_targets:
            body.append("c_%05x label byte" % at)
        line = render(ins, labels, by_addr, starts, rel, img,
                      placed, addrs, externs, fr, a)
        # **TLINK's far call**: a `call far` to a routine in the same
        # segment becomes `nop / push cs / call near` at link time, so the
        # source said `call far ptr`.
        if (ins.bytes[0] == 0xE8 and len(body) >= 2
                and body[-1].strip() == "push cs" and body[-2].strip() == "nop"
                and line.startswith("call ")):
            del body[-2:]
            tn = line[len("call "):]
            if tn[1:] in externs:
                externs[tn[1:]] = "far"
            line = "call FAR PTR " + tn
        # **TASM's own far call**: to a `proc far` in the same segment TASM
        # writes `push cs / call near` itself, so the source said `call`.
        # Forward, TASM's one pass cannot know the target is far, so there
        # the source wrote `push cs` and a near call itself - which Borland's
        # front end will not pass as `call near ptr`, so it goes as bytes.
        if (ins.bytes[0] == 0xE8 and line.startswith("call _")
                and far_procs.get(line[len("call _"):])):
            tgt = int(ins.op_str, 16)
            if tgt < ins.address and body and body[-1].strip() == "push cs":
                del body[-1]
            elif tgt > ins.address:
                line = "db 0e8h\n        dw %s-$-2" % line[len("call "):]
        body.append("        " + line)
        for t in sorted(cs_targets):
            if at < t < at + ins.size:
                body.append("c_%05x equ byte ptr $-%d" % (t, at + ins.size - t))
        if ins.mnemonic in ("ret", "retf", "iret", "jmp", "ljmp"):
            dead = True
    if open_proc:
        body.append("_%s endp" % open_proc)
    # **Outside the segment**: an `extrn` declared inside a code segment is
    # taken to be in it, and a far call to it becomes TASM's own `push cs /
    # call` - where the image has TLINK's `nop / push cs / call`, the far
    # call the linker rewrote.
    # A near one is the other way round: it is in this segment, and declared
    # outside it TASM refuses the near call as one to a different CS.
    ext = ["extrn _%s:%s" % (n, kind) for n, kind in sorted(externs.items())
           if kind == "far"]
    ext += ["extrn %s:byte" % n for n in sorted(a.data_externs)]
    near = ["extrn _%s:near" % n for n, kind in sorted(externs.items())
            if kind == "near"]
    out = ext + out[:2] + near + out[2:]
    out.extend(body)
    out.append("%s ends" % a.segment)
    if a.data:
        dlo, dhi = a.data
        dat = ["_DATA segment word public 'DATA'"]
        run = []
        base = judge.IMG_DGROUP
        for off in range(dlo, dhi):
            if off in own and run:
                dat.append("        db " + ", ".join(run))
                run = []
            if off in own:
                dat.append("d_%04x label byte" % off)
            run.append(hexnum(img[base + off]))
            if len(run) == 16:
                dat.append("        db " + ", ".join(run))
                run = []
        if run:
            dat.append("        db " + ", ".join(run))
        dat.append("_DATA ends")
        out = dat + [""] + out
    if a.bss:
        blo, bhi = a.bss
        cut = sorted({o for o in own if blo <= o < bhi} | {blo}) + [bhi]
        bss = ["_BSS segment word public 'BSS'"]
        for k in range(len(cut) - 1):
            bss.append("d_%04x db %d dup (?)" % (cut[k], cut[k + 1] - cut[k]))
        bss.append("_BSS ends")
        out = bss + [""] + out
    print("\n".join(out))


def render(ins, labels, by_addr, starts, rel, img, placed, addrs, externs,
           fr, a):
    b = ins.bytes
    m, op = ins.mnemonic, ins.op_str
    # Capstone's 16-bit mode names 99 and 98 by their 32-bit forms
    # (docs/lessons.md).
    if b[0] == 0x99:
        return "cwd"
    if b[0] == 0x98:
        return "cbw"
    opc = next(x for x in b if x not in (0x26, 0x2E, 0x36, 0x3E))
    # string instructions: the operands are the defaults, which TASM
    # writes as the bare mnemonic; an override is a prefix byte of its own
    if re.fullmatch(r"(rep[a-z]* )?(movs|stos|lods|cmps|scas|ins|outs)[bw]", m):
        pre = [x for x in b[:-1] if x in (0x26, 0x2E, 0x36)]
        if "ds:" not in op or not pre:
            return ("db %s\n        " % ", ".join(hexnum(x) for x in pre) if pre else "") + m
    # **Register `xchg`**: capstone prints the r/m operand first and TASM
    # puts its first operand in the reg field, so the two are swapped.
    if m in ("xchg", "test") and opc in (0x84, 0x85, 0x86, 0x87) and (b[-1] >> 6) == 3:
        x, y = op.split(", ")
        return "%s %s, %s" % (m, y, x)
    if m in ("lcall", "ljmp") and opc == 0xFF:
        m = "call" if m == "lcall" else "jmp"
        op = "dword ptr " + op
    # far call through a relocated segment
    if b[0] == 0x9A:
        off, seg = struct.unpack_from("<HH", b, 1)
        tgt = seg * 16 + off
        name = by_addr.get(tgt)
        if name is None:
            return "db %s  /* call far %04x:%04x, no name */" % (
                ", ".join(hexnum(x) for x in b), seg, off)
        if not (a.lo <= tgt < a.hi):
            externs[name] = "far"
        return "call FAR PTR _%s" % name
    if is_branch(ins):
        if re.fullmatch(r"0x[0-9a-f]+", op):
            t = int(op, 16)
            if t in labels:
                tn = labels[t]
            elif t in by_addr:
                tn = "_" + by_addr[t]
                if not (a.lo <= t < a.hi):
                    externs[by_addr[t]] = "near"
            else:
                return "db %s  /* %s %s outside */" % (
                    ", ".join(hexnum(x) for x in b), m, op)
            if m == "jmp":
                if b[0] == 0xEB:
                    return "jmp short %s" % tn
                # **A near jump TASM would not write as one.** Borland's
                # front end drops the size after `jmp` - `jmp near ptr x`
                # reaches the assembler as `jmp ptr x` - so the form has to
                # be one TASM picks unprompted: E9 for a target out of short
                # range. In range it would write EB, or, one-pass and
                # forward, EB and a NOP; so those are written as bytes.
                if -128 <= t - (ins.address + 2) <= 127:
                    return "db 0e9h\n        dw %s-$-2" % tn
                return "jmp %s" % tn
            if m == "call":
                return "call %s" % tn
            return "%s %s" % (m, tn)
    # segment immediates
    for k in range(1, len(b) - 1):
        if (ins.address + k) in rel:
            v = struct.unpack_from("<H", b, k)[0]
            if v == DGROUP_PARA:
                op = re.sub(r"0x%x\b" % v, "DGROUP", op)
    # capstone writes `les di, ptr [bp + 4]`; TASM wants the size
    if m in ("les", "lds"):
        op = re.sub(r"(^|, )ptr ", r"\1dword ptr ", op)
    # direct DS memory operands, and a register plus the module's own data
    mo = re.search(r"(byte|word|dword) ptr (?:ds:)?\[(?:(\w\w) \+ )?(0x[0-9a-f]+)\]", op)
    if b[0] == 0xFF and mo is None:
        mo = None
    if mo and not re.search(r"\b(es|cs|ss):\[", op) and not a.ds_is_cs:
        off = int(mo.group(3), 16)
        if (a.data and a.data[0] <= off < a.data[1]) or \
                (a.bss and a.bss[0] <= off < a.bss[1]):
            a.own.add(off)
            nm = "d_%04x" % off
        elif mo.group(2):
            nm = None
        else:
            nm = data_name(off, placed, addrs)
        if nm and mo.group(2):
            op = op.replace(mo.group(0), "%s ptr %s[%s]" % (mo.group(1), nm, mo.group(2)))
        elif nm:
            op = op.replace(mo.group(0), "%s ptr DGROUP:%s" % (mo.group(1), nm))
            if not nm.startswith("d_"):
                a.data_externs.add(nm.split("+")[0])
    # **An index register loaded with an address**: `mov si, 398ch` before
    # a `rep movsw` is VMDS+0fch, not a number. Narrow on purpose - only SI,
    # DI, BX and BP, and only a value inside an object the port places - and
    # still a guess for the reader to confirm: the bytes are the same either
    # way, so the judge cannot.
    mi = re.fullmatch(r"(si|di|bx|bp), (0x[0-9a-f]+)", op)
    if m == "mov" and mi and not re.search(r"\[", op):
        v = int(mi.group(2), 16)
        # inside the module's own data it is safe to call an address
        if (a.data and a.data[0] <= v < a.data[1]) or \
                (a.bss and a.bss[0] <= v < a.bss[1]):
            a.own.add(v)
            return "mov %s, offset DGROUP:d_%04x" % (mi.group(1), v)
        if v >= 0x100 and a.offsets:
            nm = data_name(v, placed, addrs)
            if nm:
                a.data_externs.add(nm.split("+")[0])
                return "mov %s, offset DGROUP:%s" % (mi.group(1), nm)
    # the code segment's own data, by the label the dead bytes got - through
    # a `cs:` override, or through DS where the module keeps DS on CS
    for mc in list(re.finditer(r"(cs:|(?<![a-z]:))\[(?:(\w\w) \+ )?(0x[0-9a-f]+)\]", op)):
        if mc.group(1) != "cs:" and not a.ds_is_cs:
            continue
        if mc.group(1) != "cs:" and re.search(r"\b(es|ss):\[", op):
            continue
        tgt = judge.frame_of(ins.address, fr) + int(mc.group(3), 16)
        if a.lo <= tgt < a.hi:
            rep = "%sc_%05x%s" % ("cs:" if mc.group(1) == "cs:" else "",
                                  tgt, "[%s]" % mc.group(2) if mc.group(2) else "")
            op = op.replace(mc.group(0), rep)
    op = re.sub(r"0x([0-9a-f]+)", lambda x: hexnum(int(x.group(1), 16)), op)
    op = op.replace(" + ", "+").replace(" - ", "-")
    return ("%s %s" % (m, op)).strip()


if __name__ == "__main__":
    main(sys.argv[1:])
