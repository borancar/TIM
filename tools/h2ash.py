"""The port's C structs as TASM sees them, so assembly can name fields.

    uv run python tools/h2ash.py sequence=seq sequence_channels=chan

An assembly module (`JUDGE: tasm`) reaches a record through a register and
an offset - `es:[bx+15eh]` - and the offset is a field of a struct the C
side already declares (`struct sequence`'s `volume`). This turns the C
declaration into names the assembly can use instead: `es:[bx+seq_volume]`.

**Borland's own converter does the reading.** H2ASH, which ships with
Borland C++ 3.0, reads a header the way Borland C++ does - through the
`__TURBOC__` branches, with far pointers four bytes - and writes each
struct as a TASM `STRUC`. This runs it under turboc's emulator
(`tools/tcrun.py`) on `tim.h`, with the port's headers staged as the judge
stages them, and keeps the structs asked for.

Three things are added to what H2ASH writes, because of how TASM reads it:

- **Ideal mode** (`-qi`). In MASM mode a field name is a global symbol, and
  `dgroup.h` reuses names across structs (`volume` is in two of these alone).
  In Ideal mode a field belongs to its struct.
- **A constant per field**, `alias_field = (struct PTR 0).field`, made with
  `=` so it is a number when the include is read. The modules are MASM mode
  and cannot name an Ideal-mode field directly; a constant they can. TASM
  works the offset out from the struct, so nothing here counts bytes, and a
  constant assembles exactly as the number did - `26 8A 87 015E` for
  `es:[bx+seq_volume]` and for `es:[bx+15eh]` alike, the one-byte
  displacement kept where the offset fits one (measured on TASM 3.0).
- **Only the structs named**, in the order given: H2ASH mistranslates some of
  `dgroup.h`'s others into text TASM refuses, and the include must not carry
  them. A struct a named one nests must be named too, before it.

A field may not be called by a TASM reserved word: `sequence.loop` became
`looping` for that reason.

The result is cached in out/h2ash/, keyed on the headers' contents.

This file is the port's own tooling, not a transcription.
"""
import argparse
import glob
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
RECON = os.path.join(REPO, "reconstruct")
TURBOC = os.environ.get("TIM_TURBOC", os.path.join(REPO, "..", "turboc"))
CACHE = os.path.join(REPO, "out", "h2ash")
HEADER_DIRS = (os.path.join(RECON, "src"), RECON, os.path.join(RECON, "tc"))


def _headers():
    """The headers as the judge stages them: a later directory's file of the
    same name wins, as it does there."""
    out = {}
    for where in HEADER_DIRS:
        for h in glob.glob(os.path.join(where, "*.h")):
            out[os.path.basename(h).upper()] = h
    return out


def _stage(src, dst):
    data = open(src, "rb").read().replace(b"\r\n", b"\n")
    open(dst, "wb").write(data.replace(b"\n", b"\r\n"))


def h2ash_output():
    """H2ASH's Ideal-mode translation of tim.h, as text; run once per set of
    header contents."""
    headers = _headers()
    key = hashlib.sha256()
    for name in sorted(headers):
        key.update(name.encode() + b"\0" + open(headers[name], "rb").read())
    cached = os.path.join(CACHE, key.hexdigest()[:16] + ".ash")
    if os.path.exists(cached):
        return open(cached, encoding="latin-1").read()

    d = tempfile.mkdtemp(prefix="h2ash")
    try:
        for name, path in headers.items():
            _stage(path, os.path.join(d, name))
        with open(os.path.join(d, "STRUCTS.H"), "w", newline="\r\n") as f:
            f.write('#include "tim.h"\n')
        cmd = ["uv", "run", "--project", TURBOC, "python",
               os.path.join(HERE, "tcrun.py"), "H2ASH.EXE", "--save", d]
        for name in sorted(os.listdir(d)):
            cmd += ["--add", os.path.join(d, name)]
        cmd += ["--", "-mm", "-I.", "-qi", "STRUCTS.H"]
        r = subprocess.run(cmd, cwd=TURBOC, capture_output=True, text=True,
                           env=dict(os.environ, TURBOC_VERSION="bc3.00"))
        out = os.path.join(d, "STRUCTS.ASH")
        if not os.path.exists(out):
            sys.stdout.write(r.stdout + r.stderr)
            raise SystemExit("h2ash: H2ASH wrote nothing")
        text = open(out, "rb").read().decode("latin-1").replace("\r", "")
    finally:
        shutil.rmtree(d)
    os.makedirs(CACHE, exist_ok=True)
    open(cached, "w", encoding="latin-1").write(text)
    return text


def structs_ash(wanted):
    """The include for `wanted`, a list of (struct, alias) pairs."""
    text = h2ash_output()
    blocks, consts = [], []
    for struct, alias in wanted:
        m = re.search(r"^STRUC\s+%s\s*\n(.*?)^ENDS\s+%s\s*$"
                      % (re.escape(struct), re.escape(struct)),
                      text, re.M | re.S)
        if not m:
            raise SystemExit("h2ash: no struct %s in the headers" % struct)
        blocks.append(m.group(0))
        for field in re.findall(r"^(\w+)\s+\S", m.group(1), re.M):
            consts.append("%s_%s = (%s PTR 0).%s" % (alias, field, struct, field))
    return ("; Generated by tools/h2ash.py from the port's headers.\n"
            "IDEAL\n\n" + "\n\n".join(blocks) + "\n\n" + "\n".join(consts)
            + "\n\nMASM\n")


def parse(spec):
    """`struct=alias` words, as the `JUDGE: structs` marker has them."""
    out = []
    for word in spec.split():
        struct, _, alias = word.partition("=")
        out.append((struct, alias or struct))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("structs", nargs="+", help="struct=alias, in order")
    ap.add_argument("-o", "--out", help="write here rather than to stdout")
    a = ap.parse_args()
    ash = structs_ash(parse(" ".join(a.structs)))
    if a.out:
        with open(a.out, "w", newline="\r\n") as f:
            f.write(ash)
    else:
        sys.stdout.write(ash)


if __name__ == "__main__":
    main()
