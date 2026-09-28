# SPDX-License-Identifier: GPL-2.0-only
"""**Is this source the original's?** Compile a port file with Turbo C++ 3.0 -
the compiler that built TIM.EXE - and compare every routine it defines with the
image, byte for byte.

    uv run python tools/judge.py reconstruct/src/bitmaps.c [-v]

This file is the port's own tooling, not a transcription.

**The compiler** is the host port of TCC 3.0 in the sibling `turboc`
checkout (`$TIM_TURBOC`, default `../turboc`; build it with `make tcc` in
`reconstruct/v3.00` there). Its OMF reader is that repository's
`tools/omf.py`, used as it is. The options are the ones measured on the image
(`docs/executable.md`, "What built it"): `-mm`, and nothing else until a
routine says otherwise. A file whose modules were built another way says so in
its header with `JUDGE: built-with <options>`, which replaces the defaults.

**Which routine is which** comes from the port's own provenance: the address
on the first line of the comment directly above each definition, read the way
`reconstruct/tests/provenance.py` reads it, over every source of the port - so
a far call's target can be checked against the callee's address even when the
callee is in another file.

**What is compared.** A routine is the bytes from its public symbol to the next
one in the object, against the image from its address. Where the object has a
fixup the byte is not the compiler's to decide and is masked, except that:

- a far call (`9A` and a four-byte pointer to a named routine) must reach that
  routine's address - either still as `9A seg:off`, or as TLINK left a far call
  into the caller's own segment, `nop / push cs / call near`, five bytes for
  five;
- the rest (DGROUP offsets, segment bases) are counted as unchecked and said
  so, never counted as agreement.

A routine that matches but is shorter than the gap to the next known address
is reported too: something between them is not accounted for.

**A leading `/` on TCC's command line is an option switch**, as on DOS, so the
source is copied into a scratch directory and compiled by a relative name.
"""
import argparse
import glob
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
RECON = os.path.join(REPO, "reconstruct")
TURBOC = os.environ.get("TIM_TURBOC",
                        os.path.join(os.path.dirname(REPO), "turboc"))
TC_ROOT = os.path.join(TURBOC, "dos-c")
# The compilers a module may have been built by. Two of them have host ports
# in the turboc checkout and run natively; the rest run as the originals do,
# under turboc's emulator (`tools/tcemu.py`, in turboc's own environment, so
# this repository's pinned emulator is not involved). `JUDGE: compiler
# <version>` in a file's header picks one - a turboc `$TURBOC_VERSION` name.
# Turbo C++ 3.0 is the default: its startup and runtime are the ones linked
# into the image, and the game's own modules are its.
COMPILERS = {
    "3.00": (os.path.join(TURBOC, "reconstruct", "v3.00", "tcc", "tcc"),
             os.path.join(TC_ROOT, "TCPP300", "INCLUDE")),
    "2.01": (os.path.join(TURBOC, "reconstruct", "v2.01", "tcc", "tcc"),
             os.path.join(TC_ROOT, "TC201", "INCLUDE")),
}
EMULATED = {"1.00": "TCC.EXE", "1.01": "TCC.EXE", "2.00": "TCC.EXE", "bc2.00": "BCC.EXE",
            "bc3.00": "BCC.EXE", "3.00-emu": "TCC.EXE"}
# Borland C++ 3.0 built the game - it alone turns an early `return` into a
# copy of the epilogue (`open_bit_reader`) - so it is the default. It runs
# under the emulator; `--compiler 3.00` is the host port of Turbo C++ 3.0,
# the same code generator without that pass, and much faster to iterate with.
DEFAULT_COMPILER = "bc3.00"
TCC = COMPILERS["3.00"][0]     # the host port; emulated ones need none
IMAGE = os.path.join(REPO, "out", "TIM.img")
UNPACKED = os.path.join(REPO, "out", "TIM.unpacked.exe")
DEFAULT_OPTS = ["-mm", "-O"]

sys.path.insert(0, os.path.join(TURBOC, "tools"))
sys.path.insert(0, os.path.join(RECON, "tests"))
sys.path.insert(0, HERE)
import omf                                              # noqa: E402  turboc's
import provenance                                       # noqa: E402
from cparse import parse, text                          # noqa: E402

BUILT_WITH = re.compile(r"JUDGE:\s*built-with\s+([^\n*]+)")
VIA_ASSEMBLER = re.compile(r"JUDGE:\s*via-assembler\b")
ASSEMBLER = re.compile(r"JUDGE:\s*assembler\s+(\S+)")
# **A module with inline `asm` went through the assembler** (`tcc -B`): the
# compiler writes assembly and TASM makes the object, so a jump TCC could not
# size - one across an `asm` block - is TASM's to shorten, and a one-pass
# TASM leaves a `nop` behind it. Which TASM Dynamix had is a measurement;
# turboc has host ports of four (built on demand into out/tasm/, as turboc's
# tasmbuild.py builds them) and the rest run under its emulator.
TASM_HOST = ("tasm1.00", "tasm1.01", "tasm2.00", "bc2.00")
TASM_EMULATED = ("tasm2.01", "bc3.00")
DEFAULT_ASSEMBLER = "bc2.00"
COMPILER = re.compile(r"JUDGE:\s*compiler\s+(\S+)")
# An error line from TCC (`Error file line: ...`) or TASM (`**Error** ...`),
# not TASM's summary `Error messages:    None`.
ERRORS = re.compile(r"^(\*\*)?(Error|Fatal)\b(?! messages:)", re.M)


def port_sources():
    return sorted(glob.glob(os.path.join(RECON, "src", "**", "*.c"), recursive=True) +
                  glob.glob(os.path.join(RECON, "*.c")))


def addresses(paths):
    """Every transcribed routine's name and image address, from the comment
    directly above its definition. Overlay addresses (`VGA:0x...`) are not
    image offsets and are left out."""
    out = {}
    for path in paths:
        src, root = parse(path)
        for node in root.children:
            if node.type != "function_definition":
                continue
            name = provenance.definition_name(src, node)
            block = provenance.comment_above(node)
            if name is None or block is None:
                continue
            first = provenance.first_content_line(text(src, block))
            # `0x1791b`, or `172c:065b, image 0x1791b` as the parts/*.c modules have it
            m = re.match(r"(?:[0-9a-fA-F]{4}:[0-9a-fA-F]{4},\s*image\s+)?"
                         r"(0x[0-9a-fA-F]{5})\b", first)
            if m:
                out[name] = int(m.group(1), 16)
        # **An assembly module's routines** are `proc`s inside a file-level
        # `asm { }` block, which the parse never sees - cparse blanks every
        # `#ifdef __TURBOC__` branch. Their provenance is a comment on the
        # line directly above the `proc`, and the name is the public's, less
        # the underscore C would have given it.
        for m in ASM_PROC.finditer(open(path).read()):
            out.setdefault(m.group(2), int(m.group(1), 16))
        # **A routine only the Borland branch defines** - C with inline
        # `asm` whose work the host does inside another routine - carries
        # its provenance the same way, directly above the definition.
        for m in TC_DEF.finditer(open(path).read()):
            out.setdefault(m.group(2), int(m.group(1), 16))
    return out


TC_DEF = re.compile(r"^/\* (0x[0-9a-fA-F]{5}) \*/\n[A-Za-z_][\w \t*]*?\b(\w+)\s*\(", re.M)


ASM_PROC = re.compile(r"/\*[^*]*?\b(0x[0-9a-fA-F]{5})\b[^*]*\*/[ \t]*\n"
                      r"[ \t]*_?(\w+)[ \t]+proc\b")


def turboc_aliases():
    """**The names Borland sees**: tim.h's `#define`s under `__TURBOC__` that
    give the port's names for the run-time library's own (`borland_printf`
    is `printf`) and for `main`. Answers port name -> Borland's."""
    text = open(os.path.join(RECON, "tim.h")).read()
    m = re.search(r"#ifdef __TURBOC__\n/\*\n \* \*\*The run-time library's own names.*?#endif", text, re.S)
    return dict(re.findall(r"^#define (\w+)\s+(\w+)$", m.group(0), re.M)) if m else {}


def runtime_names():
    """The C runtime's public names at their image addresses - `F_LDIV@`,
    `N_LXLSH@`, `_lseek` - so a far call into the library is checked like any
    other. Measured by turboc's libmatch.py; see tools/runtime_names.json.
    Stored without the one leading underscore the compiler adds, as the
    port's names are - one, not all: `__open` is C's `_open`, and stripping
    both made it `open`, the routine at another address."""
    j = json.load(open(os.path.join(HERE, "runtime_names.json")))
    return {(k[1:] if k.startswith("_") else k): int(v, 16)
            for k, v in j["names"].items()}


def frames():
    """The image offsets of the segment frames the far calls name: a routine
    lives in the highest one at or below it, and a `call near` is relative to
    it."""
    exe = open(UNPACKED, "rb").read()
    nrel, hdr, ro = (struct.unpack_from("<H", exe, 6)[0],
                     struct.unpack_from("<H", exe, 8)[0] * 16,
                     struct.unpack_from("<H", exe, 0x18)[0])
    img = exe[hdr:]
    seen = {0}
    for i in range(nrel):
        off, seg = struct.unpack_from("<HH", exe, ro + 4 * i)
        at = seg * 16 + off
        if at >= 3 and img[at - 3] == 0x9A:
            seen.add(struct.unpack_from("<H", img, at)[0] * 16)
    return sorted(seen)


def frame_of(addr, fr):
    return max(f for f in fr if f <= addr)


def stage(src_path, dst_path):
    """Copy a file with DOS line endings. Turbo C 2.01 ends a line at `\\r`
    and reads an LF-only header as one unterminated line; 3.0 takes either."""
    data = open(src_path, "rb").read().replace(b"\r\n", b"\n")
    open(dst_path, "wb").write(data.replace(b"\n", b"\r\n"))


def host_tasm(version):
    """turboc's host port of TASM `version`, built once into out/tasm/."""
    exe = os.path.join(REPO, "out", "tasm", version, "tasm")
    if not os.path.exists(exe):
        subprocess.run(["uv", "run", "--project", TURBOC, "python",
                        os.path.join(TURBOC, "tools", "tasmbuild.py"),
                        os.path.dirname(exe)], cwd=TURBOC, check=True,
                       capture_output=True,
                       env=dict(os.environ, TURBOC_VERSION=version))
    return exe


def assemble(d, base, opts, assembler, defines=True):
    """TASM on `base`.ASM in `d`, with the command line TCC gives it - less
    the `/D` defines for an assembly module's own source (`defines=False`),
    which never tests them, and under which TASM 3.0 in turboc's emulator
    loops until the instruction budget runs out."""
    models = {"-mt": "__TINY__", "-ms": "__SMALL__", "-mm": "__MEDIUM__",
              "-mc": "__COMPACT__", "-ml": "__LARGE__", "-mh": "__HUGE__"}
    model = next((models[o] for o in opts if o in models), "__SMALL__")
    conv = "__PASCAL__" if "-p" in opts else "__CDECL__"
    tail = ([base, "/D" + model, "/D" + conv, "/r/ml," + base, ";"] if defines
            else [base, "/r/ml," + base, ";"])
    if assembler in TASM_HOST:
        r = subprocess.run([host_tasm(assembler)] + tail, cwd=d,
                           capture_output=True, text=True)
        return r.stdout + r.stderr
    # Through tools/tcrun.py, as the compilers are: under turboc's own
    # memory figure TASM 3.0 prints its banner and then runs the emulator's
    # instruction budget out without assembling a line.
    cmd = ["uv", "run", "--project", TURBOC, "python",
           os.path.join(REPO, "tools", "tcrun.py"), "TASM.EXE",
           "--save", d, "--add", os.path.join(d, base + ".ASM"), "--"] + tail
    r = subprocess.run(cmd, cwd=TURBOC, capture_output=True, text=True,
                       env=dict(os.environ, TURBOC_VERSION=assembler))
    return r.stdout + r.stderr


TASM_ONLY = re.compile(r"JUDGE:\s*tasm\b")


def keep_obj(obj):
    """`JUDGE_KEEP_OBJ=<path>`: the object the judge built, kept - which is
    how tools/link.py links exactly what was judged."""
    keep = os.environ.get("JUDGE_KEEP_OBJ")
    if keep:
        shutil.copy(obj, keep)


def tasm_obj(path, opts, assembler):
    """**An assembly module straight to TASM** (`JUDGE: tasm`): the file's
    `#ifdef __TURBOC__` branch is `asm { }` blocks holding the module's TASM
    source, and they go to the assembler without Borland C++'s front end -
    which passes such a block through `-S` whole, but holds at most some 64K
    of a file's `asm` before it gives up with "Compiler table limit
    exceeded", and sound.c's is more. What the front end added is added
    here: the two data segments and DGROUP, which the blocks name."""
    text = open(path).read()
    branch = text[text.index("#ifdef __TURBOC__"):]
    branch = branch[:branch.index("\n#else")]
    body = "\n".join(m.group(1) for m in
                     re.finditer(r"^asm \{\n(.*?)^\}$", branch, re.M | re.S))
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    # a module that declares a data segment itself - `para`, as the video
    # interface's is - keeps its own declaration
    prelude = "".join(
        "%s segment word public '%s'\n%s ends\n" % (n, c, n)
        for n, c in (("_DATA", "DATA"), ("_BSS", "BSS"))
        if not re.search(r"^%s segment" % n, body, re.M)) + "DGROUP group _DATA,_BSS\n"
    d = tempfile.mkdtemp(prefix="judge")
    try:
        base = os.path.splitext(os.path.basename(path))[0][:8].upper()
        with open(os.path.join(d, base + ".ASM"), "w", newline="\r\n") as f:
            f.write(prelude + body + "\nend\n")
        out = assemble(d, base, opts, assembler, defines=False)
        objs = [f for f in os.listdir(d) if f.upper().endswith(".OBJ")]
        if not objs or ERRORS.search(out):
            sys.stdout.write(out)
            raise SystemExit("%s: %s did not assemble it" % (path, assembler))
        keep_obj(os.path.join(d, objs[0]))
        return omf.load(os.path.join(d, objs[0]))[0], out
    finally:
        shutil.rmtree(d)


def compile_obj(path, opts, compiler=DEFAULT_COMPILER, assembler=None):
    d = tempfile.mkdtemp(prefix="judge")
    try:
        # A DOS compiler takes an 8.3 name: `machine_draw.c` is staged as
        # `MACHINE_.C`. The name reaches the object (its module and code
        # segment names) and no byte of a routine.
        stem = os.path.splitext(os.path.basename(path))[0]
        name = stem[:8].upper() + ".C"
        stage(path, os.path.join(d, name))
        # The port's headers, staged the same way into one directory, in the
        # order a name is looked up: TCC's own stdint first.
        inc = os.path.join(d, "inc")
        os.mkdir(inc)
        for where in (os.path.join(RECON, "src"), RECON,
                      os.path.join(RECON, "tc")):
            for h in glob.glob(os.path.join(where, "*.h")):
                stage(h, os.path.join(inc, os.path.basename(h)))
        step = ["-S"] if assembler else ["-c"]
        if compiler in EMULATED:
            out = compile_emulated(d, name, inc, step + opts, compiler)
        else:
            tcc, tc_include = COMPILERS[compiler]
            r = subprocess.run([tcc] + step + opts +
                               ["-I" + inc + ";" + tc_include, name], cwd=d,
                               capture_output=True, text=True,
                               env=dict(os.environ, TURBOC_ROOT=TC_ROOT))
            out = r.stdout + r.stderr
        if assembler and not ERRORS.search(out):
            asms = [f for f in os.listdir(d) if f.upper().endswith(".ASM")]
            keep = os.environ.get("JUDGE_KEEP_ASM")
            if keep and asms:
                shutil.copy(os.path.join(d, asms[0]), keep)
            if asms:
                # The emulated TASM 3.0 loops on the `/D` defines TCC
                # hands it; the compiler's own output never tests them.
                out += assemble(d, os.path.splitext(asms[0])[0], opts,
                                assembler, defines=assembler in TASM_HOST)
        objs = [f for f in os.listdir(d) if f.upper().endswith(".OBJ")]
        if not objs or ERRORS.search(out):
            sys.stdout.write(out)
            raise SystemExit("%s: %s did not compile it" % (path, compiler))
        keep_obj(os.path.join(d, objs[0]))
        mod = omf.load(os.path.join(d, objs[0]))[0]
        # **An object without publics is a compiler that went wrong**, not a
        # file with nothing in it: BC++ 2.0 short of memory exited 0 having
        # written one (tools/tcrun.py). Every judged file defines something.
        if not mod.publics:
            sys.stdout.write(out)
            raise SystemExit("%s: %s wrote an object with no publics"
                             % (path, compiler))
        return mod, out
    finally:
        shutil.rmtree(d)


def compile_emulated(d, name, inc, opts, compiler):
    """The original compiler of turboc's `$TURBOC_VERSION` under turboc's
    emulator. The source and every header are mounted beside it (the guest
    finds `#include "tim.h"` in its current directory, and `<stdint.h>` too:
    the install's own INCLUDE has none). The object comes back through
    `--save`."""
    version = compiler.replace("-emu", "")
    mounts = [os.path.join(d, name)] + sorted(
        os.path.join(inc, h) for h in os.listdir(inc))
    # tools/tcrun.py is turboc's tcemu.py with the memory a real machine
    # left: BC++ 2.0 wrote an object without its symbol records under
    # turboc's own figure.
    cmd = ["uv", "run", "--project", TURBOC, "python",
           os.path.join(REPO, "tools", "tcrun.py"), EMULATED[compiler],
           "--save", d]
    for m in mounts:
        cmd += ["--add", m]
    # `-I.`: the mounted headers are in the current directory, which `<...>`
    # does not search on its own - and <stdint.h> is one of them.
    cmd += ["--", "-I."] + opts + [name]
    r = subprocess.run(cmd, cwd=TURBOC, capture_output=True, text=True,
                       env=dict(os.environ, TURBOC_VERSION=version))
    return r.stdout + r.stderr


def disasm(code, at, n=6):
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    lines = []
    for ins in md.disasm(bytes(code), at):
        lines.append("%05x  %-14s %s %s" % (ins.address, ins.bytes.hex(),
                                            ins.mnemonic, ins.op_str))
        if len(lines) >= n:
            break
    return lines


CS_LABEL = re.compile(r"^c_[0-9a-f]{5}$")


def judge_routine(name, seg, lo, hi, addr, img, known, fr, verbose,
                  pubs=()):
    """Compare seg.data[lo:hi] with img[addr:]. Answers (ok, first difference
    or None, unchecked fixup count, notes)."""
    fix = {}
    for off, size, kind, tgt, _self in seg.fixups:
        if lo <= off < hi:
            fix[off] = (size, kind, tgt)
    covered = set()
    for off, (size, _k, _t) in fix.items():
        covered.update(range(off, off + size))
    notes, unchecked = [], 0
    i = lo
    while i < hi:
        at = addr + (i - lo)
        b = seg.data[i]
        f = fix.get(i + 1)
        if (b == 0x9A and f and f[1] == "pointer"
                and (f[2].startswith("ext:") or f[2] == "seg:" + seg.name)):
            if f[2].startswith("ext:"):
                callee = f[2][4:].lstrip("_")
                want = known.get(callee)
            else:
                # a routine later in this same file: the fixup names this
                # code segment, and the offset is the routine's here
                disp = seg.fixinfo.get(i + 1, (0,))[0]
                target = (struct.unpack_from("<H", seg.data, i + 1)[0]
                          + disp) & 0xFFFF
                owner = [(o, n) for o, n in pubs if o <= target]
                callee, want = "?", None
                if owner:
                    o, n = owner[-1]
                    callee = n.lstrip("_")
                    if callee in known:
                        want = known[callee] + (target - o)
            if img[at] == 0x9A:
                o, s = struct.unpack_from("<HH", img, at + 1)
                got = s * 16 + o
            elif img[at:at + 3] == b"\x90\x0e\xe8":
                base = frame_of(addr, fr)
                rel = struct.unpack_from("<h", img, at + 3)[0]
                got = base + ((at + 5 - base + rel) & 0xFFFF)
            else:
                return False, i - lo, unchecked, notes
            if want is None:
                unchecked += 1
                notes.append("far call to %s (no address known) at %05x"
                             % (callee, at))
            elif want != got:
                notes.append("far call to %s reaches %05x, not %05x"
                             % (callee, got, want))
                return False, i - lo, unchecked, notes
            i += 5
            continue
        # **A near call to a routine earlier in this file** - `push cs / call`
        # - carries a displacement, and a displacement is the distance to the
        # callee in *our* layout. Compared as bytes it fails whenever anything
        # between the two differs in size, which ties every routine's verdict
        # to all the routines before it. So resolve both ends by name.
        if (b == 0x0E and i + 3 < len(seg.data) and seg.data[i + 1] == 0xE8
                and img[at] == 0x0E and img[at + 1] == 0xE8
                and not any(k in covered for k in range(i, i + 4))):
            rel = struct.unpack_from("<h", seg.data, i + 2)[0]
            target = (i + 4 + rel) & 0xFFFF
            owner = [(o, n) for o, n in pubs if o <= target]
            if owner:
                o, n = owner[-1]
                callee = n.lstrip("_")
                want = known.get(callee)
                base = frame_of(addr, fr)
                irel = struct.unpack_from("<h", img, at + 2)[0]
                got = base + ((at + 4 - base + irel) & 0xFFFF)
                if want is not None and want + (target - o) == got:
                    i += 4
                    continue
                if want is not None:
                    notes.append("near call to %s reaches %05x, not %05x"
                                 % (callee, got, want + (target - o)))
                    return False, i - lo, unchecked, notes
        # **A bare near call** to a routine of this file resolves the same
        # way. An `E8` can be an operand byte, so it is taken for a call only
        # when both ends name the same callee at the same address; otherwise
        # the bytes are compared as they are.
        if (b == 0xE8 and i + 2 < len(seg.data) and img[at] == 0xE8
                and not any(k in covered for k in range(i, i + 3))):
            rel = struct.unpack_from("<h", seg.data, i + 1)[0]
            target = (i + 3 + rel) & 0xFFFF
            owner = [(o, n) for o, n in pubs if o <= target]
            if owner:
                o, n = owner[-1]
                want = known.get(n.lstrip("_"))
                base = frame_of(addr, fr)
                irel = struct.unpack_from("<h", img, at + 1)[0]
                got = base + ((at + 3 - base + irel) & 0xFFFF)
                if want is not None and want + (target - o) == got:
                    i += 3
                    continue
        if i in covered:
            if i in fix:
                unchecked += 1
            i += 1
            continue
        if b != img[at]:
            return False, i - lo, unchecked, notes
        i += 1
    return True, None, unchecked, notes


def full_diff(ours, theirs, addr):
    """The whole routine, instruction by instruction, as a unified diff -
    branch and call targets written relative to the routine so that a
    difference early on does not make every later target differ."""
    import capstone
    import difflib
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)

    def text(code):
        out = []
        for ins in md.disasm(bytes(code), 0):
            op = ins.op_str
            if ins.mnemonic.startswith(("j", "call", "loop")) and \
                    re.fullmatch(r"0x[0-9a-f]+", op):
                op = "+%x" % int(op, 16)
            # what a fixup fills in is the linker's: a DGROUP address, a
            # far call's target
            op = re.sub(r"\[(0x)?[0-9a-f]+\]", "[mem]", op)
            if ins.mnemonic == "lcall" and "[" not in op:
                op = "far"
            out.append("%s %s" % (ins.mnemonic, op))
        return out
    a, b = text(ours), text(theirs)
    return ["  " + l for l in difflib.unified_diff(a, b, "ours", "image",
                                                   n=2, lineterm="")]


def judge(path, known, img, fr, verbose=False, force_opts=None,
          force_compiler=None, placed=None, force_assembler=None,
          full=False):
    placed = placed or {}
    src = open(path).read()
    c = COMPILER.search(src)
    compiler = force_compiler or (c.group(1) if c else DEFAULT_COMPILER)
    m = BUILT_WITH.search(src)
    opts = m.group(1).split() if m else DEFAULT_OPTS
    if force_opts is not None:
        opts = force_opts
    a = ASSEMBLER.search(src)
    assembler = (force_assembler or (a.group(1) if a else DEFAULT_ASSEMBLER)) \
        if VIA_ASSEMBLER.search(src) or force_assembler else None
    if TASM_ONLY.search(src):
        a = ASSEMBLER.search(src)
        mod, _log = tasm_obj(path, opts, force_assembler or
                             (a.group(1) if a else DEFAULT_ASSEMBLER))
    else:
        mod, _log = compile_obj(path, opts, compiler, assembler)
    results = []
    refs = []
    for si, seg in enumerate(mod.segs):
        if seg is None or seg.cls != "CODE" or not seg.length:
            continue
        pubs = sorted((off, nm) for nm, s, off in mod.publics if s == si)
        # **A `c_<address>` public is code-segment data** a sibling module
        # of the same segment names (asm2tasm.py's `--public-labels`). A run
        # of them ahead of the first routine is one unit, compared at the
        # address its first name says; one inside a routine is that
        # routine's, and does not cut its range.
        units = []
        for off, nm in pubs:
            if not (CS_LABEL.match(nm) and units):
                units.append((off, nm))
        for k, (off, nm) in enumerate(units):
            hi = units[k + 1][0] if k + 1 < len(units) else seg.length
            name = nm.lstrip("_")
            addr = known.get(name)
            if addr is None and CS_LABEL.match(nm):
                addr = int(nm[2:], 16)
            if addr is None:
                results.append((name, None, "no address", []))
                continue
            later = [a for a in known.values() if a > addr]
            gap = (min(later) - addr) if later else None
            ok, where, unchecked, notes = judge_routine(
                name, seg, off, hi, addr, img, known, fr, verbose, pubs)
            size = hi - off
            if ok and gap is not None and gap != size:
                notes.append("%d bytes compiled, %d to the next known routine"
                             % (size, gap))
            verdict = "MATCH" if ok else "DIFF at +0x%x" % where
            if ok and unchecked:
                verdict += " (%d fixups unchecked)" % unchecked
            results.append((name, addr, verdict, notes))
            if ok:
                refs.extend(data_refs(seg, off, hi, addr, img))
            if not ok and full:
                notes.extend(full_diff(seg.data[off:hi], img[addr:addr + gap
                                       if gap else addr + (hi - off) + 32],
                                       addr))
            elif not ok:
                lines_o = disasm(seg.data[off + max(0, where - 8):hi],
                                 addr + max(0, where - 8))
                lines_i = disasm(img[addr + max(0, where - 8):
                                     addr + max(0, where - 8) + 40],
                                 addr + max(0, where - 8))
                notes.append("ours:")
                notes.extend("  " + x for x in lines_o)
                notes.append("image:")
                notes.extend("  " + x for x in lines_i)
    # **A routine the file defines that the object does not hold** is one the
    # compiler never saw - a preprocessor conditional that swallowed it, as a
    # host-only block with its `#endif` in the wrong place once did three
    # routines of machine.c. Nothing above can notice: it judges what was
    # compiled, so the routine was neither a MATCH nor a DIFF, just absent.
    compiled = {nm.lstrip("_") for nm, _s, _o in mod.publics}
    compiled |= {port for port, lib in turboc_aliases().items() if lib in compiled}
    for name, addr in sorted(addresses([path]).items(), key=lambda x: x[1]):
        if name not in compiled:
            results.append((name, addr, "MISSING: defined in the file, "
                            "absent from the object the compiler wrote", []))
    results.extend(judge_data(mod, refs, img, placed,
                              JUDGED_DATA.findall(src),
                              os.path.basename(path)))
    return results


IMG_DGROUP = 0x2D3C0


def data_refs(seg, lo, hi, addr, img):
    """Every segment-relative data fixup in a matched routine, with the word
    the image holds there: (target, the object's addend, the image's word)."""
    out = []
    for off, size, kind, tgt, seg_rel in seg.fixups:
        if not (lo <= off < hi) or kind != "offset" or size != 2 \
                or not seg_rel:
            continue
        disp = seg.fixinfo.get(off, (0,))[0]
        addend = (struct.unpack_from("<H", seg.data, off)[0] + disp) & 0xFFFF
        got = struct.unpack_from("<H", img, addr + (off - lo))[0]
        out.append((tgt, addend, got))
    return out


JUDGED_DATA = re.compile(r"JUDGE:\s*data\s+0x([0-9a-fA-F]+)\.\.0x([0-9a-fA-F]+)")


def judge_data(mod, refs, img, placed, declared=(), me=None):
    """**The module's own data, placed and compared.** Each reference from a
    matched routine into the module's `_DATA` or `_BSS` says where that
    segment begins in DGROUP: the image's word less the object's addend. All
    of them must agree - a record laid out in another order, or a variable in
    the wrong module, shows up as two bases - and then `_DATA`'s bytes are
    compared with the image's at that base, fixups masked. `_BSS` is past the
    image's initialised data and only its base can be checked.

    A reference to an *extern* is checked against the port's placement of
    that object (`DGROUP_AT`/`DGROUP_BSS`), when it has one."""
    out = []
    bases = {}
    for tgt, addend, got in refs:
        if tgt.startswith("seg:"):
            seg = next((x for x in mod.segs[1:] if x.name == tgt[4:]), None)
            if seg is None or seg.cls == "CODE":
                continue    # an offset into the module's own code
            bases.setdefault(tgt[4:], set()).add((got - addend) & 0xFFFF)
        elif tgt.startswith("ext:") and tgt[4:].lstrip("_") in placed:
            want = (placed[tgt[4:].lstrip("_")] + addend) & 0xFFFF
            if want != got:
                out.append((tgt[4:], None,
                            "DIFF: referenced at %04x, placed at %04x"
                            % (got, want), []))
    # **Data nothing in the module reads** - a table file's, or a vector
    # another module jumps through - has no reference to place it by, so it
    # stands where the file declares it
    if len(declared) == 1:
        for x in mod.segs[1:]:
            if x is not None and x.cls == "DATA" and x.length and x.name not in bases:
                bases[x.name] = {int(declared[0][0], 16)}
    for segname, found in sorted(bases.items()):
        seg = next(x for x in mod.segs[1:] if x.name == segname)
        if len(found) != 1:
            out.append(("[%s]" % segname, None, "DIFF: %d bases %s"
                        % (len(found), " ".join("%04x" % b for b in sorted(found))),
                        []))
            continue
        base = found.pop()
        verdict = "at DGROUP %04x..%04x" % (base, base + seg.length)
        if seg.cls == "DATA" and declared and \
                (base, base + seg.length) not in \
                [(int(a, 16), int(b, 16)) for a, b in declared]:
            out.append(("[%s]" % segname, None,
                        "DIFF: placed %04x..%04x, the file declares %s"
                        % (base, base + seg.length,
                           ", ".join("%s..%s" % d for d in declared)), []))
            continue
        notes = []
        # **Whose bytes these are.** A literal and an extern array compile to
        # the same instruction, so a module can match with another module's
        # data claimed as its own pool - machine_draw.c did, with the strings
        # module's messages. An object another file places inside this range
        # says so.
        if me is not None:
            alien = sorted((a, n) for n, a in placed.items()
                           if base <= a < base + seg.length
                           and OWNERS.get(n) not in (None, me))
            if alien:
                out.append(("[%s]" % segname, None,
                            "DIFF: %04x..%04x holds %s, placed by %s"
                            % (base, base + seg.length, alien[0][1],
                               OWNERS[alien[0][1]]), []))
                continue
        if seg.cls == "DATA" and seg.length:
            masked = set()
            for off, size, *_ in seg.fixups:
                masked.update(range(off, off + size))
            at = IMG_DGROUP + base
            bad = [i for i in range(seg.length) if seg.have[i]
                   and i not in masked and seg.data[i] != img[at + i]]
            if bad:
                verdict = "DIFF at +0x%x, %s" % (bad[0], verdict)
                notes.append("ours : %s" % bytes(seg.data[bad[0]:bad[0] + 16]).hex(" "))
                notes.append("image: %s" % img[at + bad[0]:at + bad[0] + 16].hex(" "))
            else:
                verdict = "MATCH, " + verdict
        else:
            verdict = "MATCH, " + verdict
        out.append(("[%s]" % segname, None, verdict, notes))
    return out


# Which file places each object, for the ownership check in `judge_data`.
OWNERS = {}


def placements(paths):
    """Every object the port places in DGROUP, by name: `cparse.placements`
    over every source."""
    import cparse
    out = {}
    for p in paths:
        for _struct, name, addr in cparse.placements(p):
            out[name] = addr
            OWNERS[name] = os.path.basename(p)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("files", nargs="+")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--full", action="store_true",
                    help="a differing routine as a whole-routine diff")
    ap.add_argument("--only", help="judge only these routines (comma list)")
    ap.add_argument("--opts", help="TCC options instead of the file's own "
                    "(one string: --opts='-mm -O')")
    ap.add_argument("--assembler", choices=TASM_HOST + TASM_EMULATED,
                    help="through TASM, whether or not the file says "
                    "JUDGE: via-assembler")
    ap.add_argument("--compiler", choices=sorted(COMPILERS) + sorted(EMULATED),
                    help="instead of the file's own JUDGE: compiler")
    a = ap.parse_args(argv)
    if (a.compiler or DEFAULT_COMPILER) in COMPILERS and not os.path.exists(TCC):
        raise SystemExit("no TCC 3.0 at %s: make tcc in %s/reconstruct/v3.00"
                         % (TCC, TURBOC))
    img = open(IMAGE, "rb").read()
    known = runtime_names()
    known.update(addresses(port_sources()))
    # a routine Borland knows by the library's name has the port's address
    for port, lib in turboc_aliases().items():
        if port in known:
            known.setdefault(lib, known[port])
    # a file judged from outside the tree (a draft) names its own routines
    for extra in a.files:
        for name, addr in addresses([extra]).items():
            known.setdefault(name, addr)
    fr = frames()
    placed = placements(port_sources())
    total = matched = 0
    for path in a.files:
        for name, addr, verdict, notes in judge(
                path, known, img, fr, a.verbose,
                a.opts.split() if a.opts is not None else None,
                a.compiler, placed, a.assembler, a.full):
            if a.only and name not in a.only.split(","):
                continue
            total += 1
            matched += verdict.startswith("MATCH")
            where = "%05x" % addr if addr is not None else "  ?  "
            print("%s  %-32s %s" % (where, name, verdict))
            if a.verbose or not verdict.startswith("MATCH"):
                for n in notes:
                    print("        " + n)
    print("%d of %d routines match" % (matched, total))
    return 0 if matched == total else 1


if __name__ == "__main__":
    sys.exit(main())
