#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""**Stages a DOS build of TIM.EXE**: a directory Borland MAKE builds the
game in, under DOS, with the original compilers - no Python, no emulator of
ours, nothing but the files and the makefile.

    uv run python tools/dosbuild.py            # stage out/v<version>/dos/
    uv run python tools/dosbuild.py --run      # and build it under DOSBox

In DOS, with the Borland installs where their own TURBOC.CFG files expect
them (`C:\\BCPP300` and so on - turboc's `dos-c/` mounted as C:):

    MAKE -S                   the game as Dynamix built it, copy protection in
    MAKE -S -DCRACKED         the source's default: the program the shipped,
                              cracked TIM.EXE holds
    MAKE -S -DBC30=E:\\BC3 ... an install somewhere else (BC20, BC30, BC31,
                              TC101 and the TASMs' directories are macros)

**The makefile is generated, because three things the judge and the link do
on the host have to be done before DOS sees the files**, and they are done
here, once:

- **8.3 names.** `machine_draw.c` is not a DOS name, and cut to eight
  characters `dynamite.c` and `dynamite_plunger.c` are the same one. Every
  module is staged as `link.py`'s object name, `M<nnn>.C` or `.ASM`, with
  DOS line endings; `NAMES.TXT` says which source each is. The name reaches
  the object's module record and no byte of the program.
- **An assembly module's source** (`JUDGE: tasm`) is the `asm { }` blocks
  of its `#ifdef __TURBOC__` branch, as `judge.tasm_source` extracts them,
  and its `STRUCTS.ASH` is `tools/h2ash.py`'s - staged as `M<nnn>.ASH`,
  which the source's `INCLUDE` is pointed at.
- **One code segment per physical segment.** `link.py` renames each
  object's code segment after compiling (`one_code_segment`) so that the
  modules sharing a segment link as one; here a C module is compiled with
  `-zC<name>` and an assembly module's segment is renamed in its source,
  which comes to the same object.

The order the objects link in, and which physical segment each module is
in, are `link.py`'s, from its last run's record of where each module's data
went (`out/v<version>/link/data.json`) - so run `tools/link.py` first.

**What MAKE builds is checked, not assumed**: `--run` builds the staged
directory under DOSBox and compares its TIM.EXE with the file `link.py`
linked - the same bytes, header and all. Measured 2026-10-04: identical for
both versions, 1.00's being the original developer's file (SHA-256
`ed5d15b2...`). One 1.11 run stopped making progress in BC++ 3.1's compiler
and did not repeat; MAKE skips what is built, so a second run finishes it.

This file is the port's own tooling, not a transcription.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import judge  # noqa: E402
import link  # noqa: E402
import tim  # noqa: E402

OUT = os.path.join(tim.OUT, "dos")
DOS_C = os.path.join(link.TURBOC, "dos-c")

# A judge compiler or assembler name, as the install and the program that
# runs it: the macro is the install's directory, overridable on MAKE's
# command line, and its default is where the install's own TURBOC.CFG says
# it is.
COMPILERS = {"bc3.00": ("BC30", "BCC"), "bc2.00": ("BC20", "BCC"),
             "bc3.10": ("BC31", "BCC"), "1.01": ("TC101", "TCC")}
ASSEMBLERS = {"bc3.00": ("BC30", "TASM"), "bc2.00": ("BC20", "TASM"),
              "tasm2.01": ("TASM201", "TASM"), "tasm2.00": ("TASM200", "TASM"),
              "tasm1.01": ("TASM101", "TASM"), "tasm1.00": ("TASM100", "TASM")}
INSTALLS = {"BC20": "BCPP200", "BC30": "BCPP300", "BC31": "BCPP310",
            "TC101": "TCPP101", "TASM201": "TASM201", "TASM200": "TASM200",
            "TASM101": "TASM101", "TASM100": "TASM100"}

# DOS's command line is 127 characters, and MAKE passes a command to it as
# it is written.
DOS_LINE = 127


def crlf(text):
    return text.replace("\r\n", "\n").replace("\n", "\r\n")


def build_of(path):
    """How the judge builds a file: (compiler, options, assembler or None,
    assembly module?) - the same markers, read the same way."""
    src = open(path).read()
    c = judge.COMPILER.search(src)
    compiler = c.group(1) if c else judge.DEFAULT_COMPILER
    m = judge.BUILT_WITH.search(src)
    opts = m.group(1).split() if m else list(judge.DEFAULT_OPTS)
    a = judge.ASSEMBLER.search(src)
    assembler = a.group(1) if a else judge.DEFAULT_ASSEMBLER
    if judge.TASM_ONLY.search(src):
        return compiler, opts, assembler, True
    via = judge.VIA_ASSEMBLER.search(src)
    return compiler, opts, assembler if via else None, False


def segment_name(frame):
    """`link.one_code_segment`'s name for a physical segment."""
    return "_TEXT" if frame == 0 else "S%04X_TEXT" % (frame >> 4)


def plan():
    """Every module in link order: (path, staged base name, frame)."""
    files = sorted(link.game_files())
    base = {f: "M%03d" % k for k, f in enumerate(files)}
    saved = os.path.join(link.OUT, "data.json")
    if not os.path.exists(saved):
        raise SystemExit("%s is not there: run tools/link.py first - the link "
                         "order comes from where its build put each module's "
                         "data" % os.path.relpath(saved, tim.REPO))
    rec = {os.path.join(tim.REPO, k): v for k, v in json.load(open(saved)).items()}
    missing = [f for f in files if f not in rec]
    if missing:
        raise SystemExit("data.json does not know %s: run tools/link.py again"
                         % ", ".join(os.path.relpath(f, tim.REPO) for f in missing))
    code = {f: link.first_address(f) for f in files}
    fr = judge.frames()
    order = link.object_order([(f, code[f], rec[f]["data"]) for f in files], fr,
                              {f: rec[f].get("bss") for f in files})
    return [(f, base[f], judge.frame_of(addr, fr) if addr is not None else 0)
            for f, addr, _d in order]


def stage(out):
    if os.path.exists(out):
        shutil.rmtree(out)
    inc = os.path.join(out, "INC")
    os.makedirs(inc)
    for where in (os.path.join(judge.RECON, "src"), judge.RECON,
                  os.path.join(judge.RECON, "tc")):
        for h in sorted(os.listdir(where)):
            if h.endswith(".h"):
                judge.stage(os.path.join(where, h), os.path.join(inc, h.upper()))
    headers = " ".join("INC\\" + h for h in sorted(os.listdir(inc)))

    names, rules, objs, used = [], [], [], set()
    for path, base, frame in plan():
        rel = os.path.relpath(path, tim.REPO)
        compiler, opts, assembler, tasm_only = build_of(path)
        seg = segment_name(frame)
        obj = base + ".OBJ"
        objs.append(obj)
        names.append("%s  %s  %s" % (base, seg, rel))
        if tasm_only:
            source, structs = judge.tasm_source(path)
            # the module's one code segment, renamed as link.py renames it
            own = set(re.findall(r"^(\w*_TEXT)\s+segment\b", source, re.M | re.I))
            if len(own) != 1:
                raise SystemExit("%s: %d code segments, not one" % (rel, len(own)))
            source = re.sub(r"\b%s\b" % re.escape(own.pop()), seg, source)
            if structs:
                import h2ash
                ash = h2ash.structs_ash(h2ash.parse(structs))
                open(os.path.join(out, base + ".ASH"), "w", newline="").write(crlf(ash))
                source = re.sub(r"^INCLUDE STRUCTS\.ASH$", "INCLUDE %s.ASH" % base,
                                source, flags=re.M)
            open(os.path.join(out, base + ".ASM"), "w", newline="").write(crlf(source))
            macro, prog = ASSEMBLERS[assembler]
            used.add(macro)
            deps = base + ".ASM" + (" %s.ASH" % base if structs else "")
            cmds = ["$(%s)\\BIN\\%s /r /ml %s.ASM, %s" % (macro, prog, base, obj)]
        else:
            judge.stage(path, os.path.join(out, base + ".C"))
            macro, prog = COMPILERS[compiler]
            used.add(macro)
            opts = [o for o in opts if not o.startswith("-zC")] + ["-zC" + seg]
            deps = base + ".C " + headers
            inc_path = "-IINC;$(%s)\\INCLUDE" % macro
            if assembler is None:
                cmds = ["$(%s)\\BIN\\%s -c %s $(DEFS) %s %s.C"
                        % (macro, prog, " ".join(opts), inc_path, base)]
            else:
                amacro, aprog = ASSEMBLERS[assembler]
                used.add(amacro)
                cmds = ["$(%s)\\BIN\\%s -S %s $(DEFS) %s %s.C"
                        % (macro, prog, " ".join(opts), inc_path, base),
                        "$(%s)\\BIN\\%s /D__MEDIUM__ /D__CDECL__ /r /ml %s.ASM, %s"
                        % (amacro, aprog, base, obj)]
        rules.append("# %s\n%s: %s\n%s\n" % (rel, obj, deps,
                                               "\n".join("\t" + c for c in cmds)))
        # what DOS will be handed, with the longest a default can make it
        for c in cmds:
            longest = re.sub(r"\$\((\w+)\)", lambda m: "C:\\" + INSTALLS.get(m.group(1), "X" * 8),
                             c.replace("$(DEFS)", "-DTIM_COPY_PROTECTION"))
            if len(longest) > DOS_LINE:
                raise SystemExit("%s: a %d-character command, more than DOS "
                                 "takes:\n%s" % (rel, len(longest), longest))

    lines, cur = [], ""
    for n in ["$(BC30)\\LIB\\C0M.OBJ"] + objs:
        if len(cur) + len(n) + 3 > 100:
            lines.append(cur + " +")
            cur = ""
        cur += (" " if cur else "") + n
    lines.append(cur)
    macros = sorted(used | {"BC30"})
    mk = ["# TIM.EXE %s, for Borland MAKE (MAKE -S). Generated by" % tim.VERSION,
          "# tools/dosbuild.py from reconstruct/v%s - regenerate rather than edit."
          % tim.VERSION,
          "#",
          "#   MAKE -S            as Dynamix built it, the copy protection in",
          "#   MAKE -S -DCRACKED  the source's default, the shipped crack's program",
          "#",
          "# NAMES.TXT says which source each M<nnn> is and its code segment.",
          ""]
    for m in macros:
        mk.append("!if !$d(%s)\n%s = C:\\%s\n!endif" % (m, m, INSTALLS[m]))
    mk += ["",
           "!if $d(CRACKED)",
           "DEFS =",
           "!else",
           "DEFS = -DTIM_COPY_PROTECTION",
           "!endif",
           "",
           "# C0M.OBJ first, the modules in the order DGROUP's data says, CM.LIB;",
           "# /i writes the zero-initialised segments and the stack into the file.",
           "TIM.EXE: " + " \\\n\t".join(" ".join(objs[i:i + 10])
                                         for i in range(0, len(objs), 10)),
           "\t$(BC30)\\BIN\\TLINK /c /m /s /i @&&|",
           *lines,
           "TIM",
           "TIM",
           "$(BC30)\\LIB\\CM.LIB",
           "|",
           ""] + rules
    open(os.path.join(out, "MAKEFILE"), "w", newline="").write(crlf("\n".join(mk)))
    open(os.path.join(out, "NAMES.TXT"), "w", newline="").write(crlf("\n".join(names) + "\n"))
    return len(objs)


def run(out, cracked):
    """MAKE in DOSBox: turboc's installs on C:, the staged directory on D:.
    Answers the TIM.EXE it built, or None, and MAKE's log."""
    for f in ("TIM.EXE", "TIM.MAP", "MAKE.LOG"):
        if os.path.exists(os.path.join(out, f)):
            os.remove(os.path.join(out, f))
    for f in os.listdir(out):
        if f.upper().endswith(".OBJ"):
            os.remove(os.path.join(out, f))
    make = "C:\\BCPP300\\BIN\\MAKE -S%s > MAKE.LOG" % (" -DCRACKED" if cracked else "")
    conf = os.path.join(out, "DOSBOX.CNF")
    open(conf, "w").write("[sdl]\nfullscreen=false\n[dosbox]\nmemsize=16\n"
                          "[cpu]\ncycles=max\ncore=dynamic\n[mixer]\nnosound=true\n"
                          "[autoexec]\nmount c \"%s\"\nmount d \"%s\"\nd:\n%s\nexit\n"
                          % (DOS_C, out, make))
    # "offscreen": DOSBox Staging refuses "dummy", which has no renderer
    env = dict(os.environ, SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
    subprocess.run(["dosbox", "-conf", conf, "-noconsole"], env=env,
                   capture_output=True, timeout=3600)
    log = os.path.join(out, "MAKE.LOG")
    text = open(log, "rb").read().decode("cp437") if os.path.exists(log) else ""
    exe = os.path.join(out, "TIM.EXE")
    return (exe if os.path.exists(exe) else None), text


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=OUT, help="where to stage (default "
                    "out/v<version>/dos)")
    ap.add_argument("--run", action="store_true",
                    help="build it with Borland MAKE under DOSBox and compare "
                    "with tools/link.py's TIM.EXE")
    ap.add_argument("--cracked", action="store_true",
                    help="with --run: MAKE -DCRACKED, compared with link.py "
                    "--cracked's file")
    a = ap.parse_args(argv)
    n = stage(a.out)
    print("staged %d modules and a MAKEFILE in %s" % (n, os.path.relpath(a.out, tim.REPO)))
    if not a.run:
        return 0
    exe, log = run(a.out, a.cracked)
    if exe is None:
        print(log[-3000:])
        raise SystemExit("MAKE wrote no TIM.EXE")
    ref = os.path.join(link.OUT, "exe-cracked" if a.cracked else "exe", "TIM.EXE")
    built, linked = open(exe, "rb").read(), open(ref, "rb").read()
    if a.cracked:
        # link.py gives the cracked build the minalloc LZEXE's unpacker left
        # (CRACKER_MINALLOC), which is no part of what TLINK writes
        linked = linked[:10] + built[10:12] + linked[12:]
    print("MAKE's TIM.EXE  sha256 %s" % hashlib.sha256(built).hexdigest())
    if built == linked:
        print("IDENTICAL to tools/link.py's %s" % os.path.relpath(ref, tim.REPO))
        return 0
    diff = next(i for i in range(min(len(built), len(linked)))
                if built[i] != linked[i]) if len(built) == len(linked) else None
    print("differs from tools/link.py's %s (%s)" % (
        os.path.relpath(ref, tim.REPO),
        "first at byte %#x" % diff if diff is not None
        else "%d bytes against %d" % (len(built), len(linked))))
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
