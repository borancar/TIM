#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""**Drafts an assembly module as C with inline `asm`**, for the judge to
prove or refute.

Reads the TASM source in a port file's `#ifdef __TURBOC__` block (the one
`asm2tasm.py` drafted) and writes each `proc` as a C function whose body is
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

Usage: asm2c.py <port file> [-o <out.c>] [--opts '-mm -k-']
"""
import argparse
import re
import sys

PROC = re.compile(r"^(/\* 0x[0-9a-f]{5} \*/)\n_(\w+) proc (near|far)\n(.*?)^_\2 endp",
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


def convert_proc(name, kind, body, why, header=""):
    ls = body_lines(body)
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
            out.append("    asm " + c_operand(l))
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
    ap.add_argument("--install", action="store_true",
                    help="rewrite the port file: the asm block becomes the "
                         "functions, the markers say C through TASM")
    a = ap.parse_args(argv)
    s = open(a.file).read()
    blk = s[s.index("asm {"):s.index("\n}\n#else")]
    if a.data:
        labs = re.findall(r"^(d_([0-9a-f]{4})) label", blk, re.M)
        base = int(labs[0][1], 16)
        for lab, off in labs:
            k = int(off, 16) - base
            DATA_MAP[lab] = a.data + ("+%d" % k if k else "")
    externs = re.findall(r"^extrn _(\w+):(\w+)", blk, re.M)
    procs = PROC.findall(blk)
    header = open("reconstruct/tim.h").read()
    why = []
    funcs = []
    for prov, name, kind, body in procs:
        c = convert_proc(name, kind, body, why, header)
        if c:
            funcs.append(c[0] + prov + "\n" + c[1] + c[2])
    if why:
        sys.stderr.write("not C:\n  " + "\n  ".join(why) + "\n")
        return 1
    if a.install:
        return install(a.file, s, funcs, externs, a.opts)
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


def install(path, s, funcs, externs, opts):
    i = s.index("#ifdef __TURBOC__\n") + len("#ifdef __TURBOC__\n")
    j = s.index("\n}\n#else\n") + len("\n}\n")
    decl = []
    for n, k in externs:
        if k not in ("far", "near") and not re.search(r"\b%s\b" % n, open("reconstruct/dgroup.h").read()):
            decl.append("extern char %s[];" % n)
    new = ("/*\n * C with inline `asm`, compiled through TASM (`JUDGE: via-assembler`):\n"
           " * the frame, the SI/DI save and each final return are the compiler's.\n"
           " * Drafted by tools/asm2c.py.\n */\n" + ("\n".join(decl) + "\n\n" if decl else "")
           + "\n".join(funcs))
    s = s[:i] + new + s[j:]
    s = re.sub(r" \* JUDGE: built-with .*\n \* JUDGE: tasm\n \* JUDGE: assembler (\S+)\n",
               lambda m: " * JUDGE: compiler bc3.00\n * JUDGE: built-with %s\n"
                         " * JUDGE: via-assembler\n * JUDGE: assembler %s\n" % (opts, m.group(1)), s)
    open(path, "w").write(s)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
