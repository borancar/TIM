#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""**Drafts an assembly module as C with inline `asm`**, for the judge to
prove or refute.

Reads an assembly module's TASM source, its `.asm` (the one `asm2tasm.py`
drafted), and writes each `proc` as a C function whose body is
`asm` statements, labels as C labels. What Borland C++ writes itself is left
to it, so a match is evidence of C and not of transcription:

  - `push bp / mov bp, sp` and the `pop bp` before the last return: the
    compiler's frame, which with `-k-` it builds for a function that has
    parameters - so a routine with a frame gets one, `a`, named nowhere;
  - `push si` / `push di` after the frame and their pops before the last
    return, in that order: what it saves for any function that names SI or
    DI;
  - the last `ret` or `retf`, which it always writes, even after an `asm`
    jump.

A routine that does not end in a return cannot be C (the compiler would add
one), and one that names SI or DI outside that save cannot be either; both
are refused, and the module stays assembly. See docs/lessons.md.

Usage: asm2c.py <module.asm or its .c> [-o <out.c>] [--opts '-mm -k-']
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from version import RECON  # noqa: E402

PROC = re.compile(r"^;[^\n]*?(0x[0-9a-f]{5})[^\n]*\n_(\w+) proc (near|far)\n(.*?)^_\2 endp",
                  re.M | re.S)


DATA_MAP = {}


def c_operand(line):
    """An operand as the C file says it: a DGROUP or external symbol by its
    C name, which the compiler hands to TASM as its own reference; the
    module's own `_DATA` labels as offsets into the C object that holds it
    (`--data`)."""
    for lab, ref in DATA_MAP.items():
        line = re.sub(r"DGROUP:%s\b" % lab, ref, line)
    line = re.sub(r"DGROUP:_(\w+)", r"\1", line)
    line = re.sub(r"FAR PTR _(\w+)", r"far ptr \1", line)
    line = re.sub(r"\bcall _(\w+)", r"call near ptr \1", line)
    line = re.sub(r"\b_(\w+)\b", r"\1", line)
    # `name[si]` on a C array loses the array's address to the compiler;
    # `[si+name]` keeps it
    line = re.sub(r"\b([A-Za-z_]\w*)\[(si|di|bx|bp)((?:[+-][^\]]*)?)\]",
                  lambda m: m.group(0) if m.group(1) in ("ptr", "cs", "ds", "es", "ss")
                  else "[%s%s+%s]" % (m.group(2), m.group(3), m.group(1)), line)
    return line


def body_lines(body):
    return [l.strip() for l in body.split("\n") if l.strip()]


def prototype(name, header):
    """tim.h's declaration of `name`, as `ret name(params)`, or None."""
    m = re.search(r"^([A-Za-z_][\w \t*]*?\b%s\s*\((?:[^()]|\([^()]*\))*\))\s*;"
                  % re.escape(name), header, re.M)
    if not m:
        return None
    return re.sub(r"\s+", " ", m.group(1)).replace("( ", "(")


OWN = set()        # labels the module defines for itself (--partial)
DROPPED = [0]      # `db` runs of undecoded code set aside (--partial)
CALLED = set()     # labels of other routines called from the asm


def sized(line):
    """A module label with no size of its own, stored to or loaded from a
    register, gets the register's: `extern char` makes TASM refuse a word."""
    m = re.match(r"^(\w+) (.*)$", line)
    if not m or " ptr " in line:
        return line
    ops = [x.strip() for x in m.group(2).split(",")]
    if len(ops) != 2:
        return line
    w = {"ax", "bx", "cx", "dx", "si", "di", "bp", "sp"}
    b = {"al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"}
    for i, op in enumerate(ops):
        base = re.match(r"^(\w+)", op)
        if base and base.group(1) in OWN:
            other = ops[1 - i]
            size = "word" if other in w else "byte" if other in b else None
            if size:
                ops[i] = "%s ptr %s" % (size, op)
                return "%s %s" % (m.group(1), ", ".join(ops))
    return line


MIDCALL = {}       # routine -> labels it calls in the middle of another


def convert_proc(name, kind, body, why, header=""):
    if name in MIDCALL:
        why.append("%s calls into the middle of another routine (%s)"
                   % (name, ", ".join(MIDCALL[name])))
        return None
    ls = body_lines(body)
    # **A jump into another routine** is not something inline `asm` can
    # make: its labels are its own function's.
    here = {l[:-1] for l in ls if l.endswith(":")}
    for l in ls:
        m = re.match(r"^(j\w+|loop\w*) (?:short |near ptr )?(L[0-9a-f]+|_\w+)$", l)
        if m and m.group(2).lstrip("_") not in here and m.group(2) not in here:
            why.append("%s jumps into another routine (%s)" % (name, m.group(2)))
            return None
        m = re.match(r"^call (?:near ptr )?(L[0-9a-f]+)$", l)
        if m and m.group(1) not in here:
            CALLED.add(m.group(1))
    ins = [l for l in ls if not l.endswith(":")]
    if not ins or not re.match(r"^retf?\b", ins[-1]):
        why.append("%s does not end in a return" % name)
        return None
    frame = ls[:2] == ["push bp", "mov bp, sp"]
    if frame:
        ls = ls[2:]
    # **Locals**: `sub sp, N` is N bytes of them, and the compiler restores
    # SP with `mov sp, bp` before it pops BP. The `asm` names them by their
    # offsets, so only their size is the C's.
    locals_ = 0
    if frame and ls and re.match(r"^sub sp, (\w+)$", ls[0]):
        v = re.match(r"^sub sp, (\w+)$", ls[0]).group(1)
        locals_ = int(v[:-1], 16) if v.endswith("h") else int(v)
        ls = ls[1:]
    saved = []
    while ls and ls[0] in ("push si", "push di") and \
            (not saved or (saved == ["si"] and ls[0] == "push di")):
        saved.append(ls[0].split()[1])
        ls = ls[1:]
    # what the body names, the epilogue's own pops left out
    text = " ".join(ls[:len(ls) - 1 - len(saved) - (1 if frame else 0) - (1 if locals_ else 0)])
    uses = [r for r in ("si", "di") if re.search(r"\b%s\b" % r, text)]
    # **A saved register the body never names** is a `register` variable
    # the C declared: BC++ 3.0 puts the first in SI and saves it whether or
    # not anything uses it. Only SI can be that - DI goes to a second one.
    regvar = saved == ["si", "di"] and uses == ["di"]
    if not regvar and uses != saved:
        why.append("%s names %s outside the compiler's save" % (name, uses or saved))
        return None
    # the tail: [pop di] [pop si] [pop bp] ret(f), labels allowed before it
    tail = ["pop %s" % r for r in reversed(saved)] + (["mov sp, bp"] if locals_ else []) + (["pop bp"] if frame else [])
    last = ls.pop()
    if not re.match(r"^retf?$", last) or (last == "retf") != (kind == "far"):
        why.append("%s ends in `%s`, which the compiler would not write" % (name, last))
        return None
    for t in reversed(tail):
        if not ls or ls[-1] != t:
            why.append("%s does not end in the compiler's epilogue" % name)
            return None
        ls.pop()
    # labels left at the end stand before the compiler's epilogue
    out = []
    for l in ls:
        if l.endswith(":"):
            out.append(l)
        else:
            line = sized(c_operand(l))
            if locals_:
                # the locals by name, so the compiler keeps them: `[bp-k]`
                # is `locals+(N-k)`, the same address
                line = re.sub(r"\[bp-(\w+)\]", lambda m: "locals+%d" % (
                    locals_ - (int(m.group(1)[:-1], 16) if m.group(1).endswith("h")
                               else int(m.group(1)))), line)
            out.append("    asm " + line)
    if out and out[-1].endswith(":"):
        out.append("    ;")
    proto = prototype(name, header)
    if proto:
        has_params = not re.search(r"\(\s*void\s*\)$", proto)
        if has_params and not frame:
            why.append("%s: tim.h says %s, and the image has no frame"
                       % (name, proto))
            return None
        # **A frame and no parameters**: `-k-` builds none, so the function
        # asked for one - `#pragma option -k`, which Borland C++ takes
        # between functions.
        pragma = frame and not has_params
        ret, rest = proto.split(name, 1)
        dist = "" if kind == "far" or re.search(r"\bnear\b", ret) else "near "
        head = "%s%s%s%s" % (ret, dist, name, rest)
    else:
        head = "void %s %s(%s)" % (kind, name, "int a" if frame else "void")
        pragma = False
    if locals_:
        out.insert(0, "    char locals[%d];\n" % locals_)
    if regvar:
        out.insert(0, "    register int held;\n")
    fn = "%s\n{\n%s\n}\n" % (head, "\n".join(out))
    return ("#pragma option -k\n" if pragma else "", fn,
            "#pragma option -k-\n" if pragma else "")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("-o", "--out")
    ap.add_argument("--opts", default="-mm -k-")
    ap.add_argument("--data", help="the C object the module's _DATA is: "
                    "its d_ labels become offsets into it, and its "
                    "definition is written by hand in the C branch")
    ap.add_argument("--partial", action="store_true",
                    help="draft the routines that can be C and list the "
                         "rest, with generic signatures and every module "
                         "label external, for the judge to count")
    ap.add_argument("--install", action="store_true",
                    help="rewrite the port file: the asm block becomes the "
                         "functions, the markers say C through TASM")
    a = ap.parse_args(argv)
    asm = os.path.splitext(a.file)[0] + ".asm"
    s = blk = open(asm).read()
    if a.data:
        labs = re.findall(r"^(d_([0-9a-f]{4})) label", blk, re.M)
        base = int(labs[0][1], 16)
        for lab, off in labs:
            k = int(off, 16) - base
            DATA_MAP[lab] = a.data + ("+%d" % k if k else "")
    externs = re.findall(r"^extrn _(\w+):(\w+)", blk, re.M)
    procs = PROC.findall(blk)
    if a.partial:
        procs, DROPPED[0] = split_procs(blk, procs)
        procs = [(p, n, k or "near", b) for p, n, k, b in procs]
        # **A call to a label that starts no routine** is a call into the
        # middle of one, which C cannot make; its caller is refused
        entries = {n for _p, n, _k, _b in procs}
        mid = []
        for p_, n, k, b in procs:
            bad = [t for t in re.findall(r"\bcall (?:near ptr |FAR PTR |far ptr )?(L[0-9a-f]+)\b", b)
                   if t not in entries]
            if bad:
                MIDCALL[n] = bad
    header = "" if a.partial else open(os.path.join(RECON, "tim.h")).read()
    OWN.update(re.findall(r"^([A-Za-z_]\w*)\s+(?:label|db|dw|dd|equ)\b", blk, re.M))
    OWN.update(re.findall(r"^\s*([cd]_[0-9a-f]+)\b", blk, re.M))
    why = []
    funcs = []
    for prov, name, kind, body in procs:
        c = convert_proc(name, kind, body, why, header)
        if c:
            funcs.append(c[0] + "/* %s */\n" % prov + c[1] + c[2])
    if a.partial:
        return partial(a, blk, externs, procs, funcs, why)
    if why:
        sys.stderr.write("not C:\n  " + "\n  ".join(why) + "\n")
        return 1
    if a.install:
        return install(os.path.splitext(a.file)[0] + ".c", asm, funcs, externs, a.opts)
    out = ["/*", " * Drafted by tools/asm2c.py from %s." % a.file, " *",
           " * JUDGE: compiler bc3.00", " * JUDGE: built-with %s" % a.opts,
           " * JUDGE: via-assembler", " * JUDGE: assembler bc3.00", " */"]
    for n, k in externs:
        if k in ("far", "near"):
            out.append("void %s %s(void);" % (k, n))
        else:
            out.append("extern char %s[];" % n)
    for _prov, name, kind, body in procs:
        frame = body_lines(body)[:2] == ["push bp", "mov bp, sp"]
        out.append("void %s %s(%s);" % (kind, name, "int a" if frame else "void"))
    out.append("")
    out.append("\n".join(funcs))
    text = "\n".join(out)
    if a.out:
        open(a.out, "w").write(text)
    else:
        sys.stdout.write(text)
    return 0


def split_procs(blk, procs):
    """**The routines a `proc` holds**, for `--partial`: a label something
    calls, standing after an unconditional return or jump, starts one of
    its own (the draft lumps what nothing names); a `db` run after a return
    is code nothing decoded, set aside and counted apart. Answers
    (address, name, kind, body) and the number of `db` runs dropped."""
    called = set(re.findall(r"\bcall (?:near ptr |FAR PTR |far ptr )?(L[0-9a-f]+)\b", blk))
    out, dropped = [], 0
    for prov, name, kind, body in procs:
        lines = body.split("\n")
        cur_name, cur_addr, cur = name, prov, []
        prev = ""
        i = 0
        while i < len(lines):
            l = lines[i].strip()
            after_exit = re.match(r"^(retf?|iret|jmp)\b", prev) is not None
            if after_exit and l.endswith(":") and l[:-1] in called:
                out.append((cur_addr, cur_name, cur))
                cur_name, cur_addr, cur = l[:-1], "0x" + l[1:-1], []
                i += 1
                continue
            if after_exit and re.match(r"^(c_[0-9a-f]+ label byte|(c_[0-9a-f]+ )?db\b)", l):
                # bytes after an exit - undecoded code, a pad, a table: skip
                # them, with any label, up to the next instruction or label
                j = i + (1 if "label byte" in l else 0)
                while j < len(lines) and re.match(r"^(c_[0-9a-f]+ label byte$|(c_[0-9a-f]+ )?db\b)", lines[j].strip()):
                    j += 1
                rest = [x.strip() for x in lines[j:] if x.strip()]
                trailing = not rest or (rest[0].endswith(":") and rest[0][:-1] in called)
                if j > i and trailing:
                    dropped += 1
                    i = j
                    continue
            if l:
                cur.append(lines[i])
                if not l.endswith(":") and not l.startswith("c_"):
                    prev = l
            i += 1
        out.append((cur_addr, cur_name, cur))
    res = []
    for addr, n, body in out:
        text = "\n".join(body) + "\n"
        ins = [x.strip() for x in body if x.strip() and not x.strip().endswith(":")]
        last = ins[-1] if ins else ""
        k = "far" if last.startswith("retf") else "near" if last.startswith("ret") else None
        res.append((addr, n, k, text))
    return res, dropped


def partial(a, blk, externs, procs, funcs, why):
    """The draft `--partial` writes: what converts, with every label the
    module defines for itself declared external so its references are
    fixups the judge masks; the refusals go to stderr, one a line."""
    out = ["/*", " * JUDGE: compiler bc3.00", " * JUDGE: built-with %s" % a.opts,
           " * JUDGE: via-assembler", " * JUDGE: assembler bc3.00", " */"]
    procnames = {n for _p, n, _k, _b in procs}
    own = OWN - procnames
    for n in sorted(CALLED):
        out.append("void near %s(void);" % n)
    for n in sorted(own):
        out.append("extern char %s[];" % n)
    for n, k in externs:
        if k in ("far", "near"):
            out.append("void %s %s(void);" % (k, n))
        elif n not in own:
            out.append("extern char %s[];" % n)
    for _prov, name, kind, body in procs:
        frame = body_lines(body)[:2] == ["push bp", "mov bp, sp"]
        out.append("void %s %s(%s);" % (kind, name, "int a" if frame else "void"))
    out.append("")
    out.append("\n".join(funcs))
    open(a.out, "w").write("\n".join(out))
    for w in why:
        sys.stderr.write(w + "\n")
    sys.stderr.write("# %d routines, %d undecoded db runs set aside\n"
                     % (len(procs), DROPPED[0]))
    return 0


def install(path, asm, funcs, externs, opts):
    """The module becomes C: the functions go into its `.c` as the
    `#ifdef __TURBOC__` branch beside the host's transcription, the `.asm`'s
    markers say C through TASM in the `.c`'s header, and the `.asm` goes."""
    s = open(path).read()
    src = open(asm).read()
    decl = []
    for n, k in externs:
        if k not in ("far", "near") and not re.search(r"\b%s\b" % n, open(os.path.join(RECON, "dgroup.h")).read()):
            decl.append("extern char %s[];" % n)
    new = ("/*\n * C with inline `asm`, compiled through TASM (`JUDGE: via-assembler`):\n"
           " * the frame, the SI/DI save and each final return are the compiler's.\n"
           " * Drafted by tools/asm2c.py.\n */\n" + ("\n".join(decl) + "\n\n" if decl else "")
           + "\n".join(funcs))
    # after the includes, the Borland branch; the host's code is the #else
    inc = list(re.finditer(r"^#include .*\n", s, re.M))[-1].end()
    s = s[:inc] + "\n#ifdef __TURBOC__\n" + new + "\n#else\n" + s[inc:] + "#endif\n"
    m = re.search(r"^; JUDGE: assembler (\S+)$", src, re.M)
    markers = (" *\n * JUDGE: compiler bc3.00\n * JUDGE: built-with %s\n"
               " * JUDGE: via-assembler\n * JUDGE: assembler %s\n"
               % (opts, m.group(1) if m else "bc3.00"))
    end = s.index(" */\n")
    s = s[:end] + markers + s[end:]
    open(path, "w").write(s)
    os.remove(asm)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
