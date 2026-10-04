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
- **A constant per field**, `alias_field = OFFSET (struct PTR 0).field`,
  made with `=` so it is a number when the include is read. Without the
  `OFFSET` the constant keeps the field's type, and `mov ax, [bx+c]` on a
  dword field is refused for operand size. The modules are MASM mode
  and cannot name an Ideal-mode field directly; a constant they can. TASM
  works the offset out from the struct, so nothing here counts bytes, and a
  constant assembles exactly as the number did - `26 8A 87 015E` for
  `es:[bx+seq_volume]` and for `es:[bx+15eh]` alike, the one-byte
  displacement kept where the offset fits one (measured on TASM 3.0).
- **Only the structs named**, in the order given: H2ASH mistranslates some of
  `dgroup.h`'s others into text TASM refuses, and the include must not carry
  them. A struct a named one nests must be named too, before it.

A word `PREFIX*` among the structs brings the numeric `#define`s of that
prefix as `EQU`s, read from the headers rather than H2ASH's (see
`defines`): `VM_SLOT_*` names the video driver's vector slots, so a thunk
jumps through `_g_vm_driver+vmdrv_entry+4*VM_SLOT_SHOW_PAGE`.

A field may not be called by a TASM reserved word: `sequence.loop` became
`looping`, and `resource.in`/`end` `in_pos`/`in_end`, for that reason. A
union's fields lose H2ASH's `FAR PTR`, which TASM counts as a value and
then refuses the union's instance for having more than one.

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
sys.path.insert(0, HERE)
from version import REPO, RECON  # noqa: E402
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


def _block(text, struct):
    m = re.search(r"^(?:STRUC|UNION)\s+%s\s*\n(.*?)^ENDS\s+%s\s*$"
                  % (re.escape(struct), re.escape(struct)), text, re.M | re.S)
    if not m:
        raise SystemExit("h2ash: no struct %s in the headers" % struct)
    return m


def _c_struct_body(struct):
    """A struct's body as the headers declare it."""
    for path in _headers().values():
        m = re.search(r"^struct %s \{(.*?)^\}" % re.escape(struct),
                      open(path).read(), re.M | re.S)
        if m:
            return m.group(1)
    # Declared inside another struct (`struct timer_tick { ... } tick[16]`):
    # what an array of it needs is read from that struct instead.
    return ""


def _struct_arrays(struct, block):
    """**H2ASH drops the count of an array of structs**: `struct timer_tick
    tick[16]` comes out as `tick timer_tick <>`, one element, which would put
    every field after it at the wrong offset and the struct at the wrong
    size. The count is put back from the C declaration - `struct T name[N]`
    or the inline `struct T { ... } name[N]`."""
    body = _c_struct_body(struct)

    def fix(m):
        field, gap, typ = m.group(1), m.group(2), m.group(3)
        a = (re.search(r"\bstruct\s+%s\s+(?:far\s+)?%s\s*\[\s*(\w+)\s*\]"
                       % (re.escape(typ), re.escape(field)), body)
             or re.search(r"\}\s*%s\s*\[\s*(\w+)\s*\]" % re.escape(field), body))
        if not a:
            return m.group(0)
        return "%s%s%s %d DUP (<>)" % (field, gap, typ, int(a.group(1), 0))

    return re.sub(r"^(\w+)(\s+)([\w$]+)\s+<>", fix, block, flags=re.M)


def _union_body(block):
    """A union's fields without H2ASH's `NEAR PTR`/`FAR PTR`: TASM counts
    `FAR PTR ?` as a value, and then refuses any instance of the union
    for having more than one."""
    return re.sub(r"\b(?:NEAR|FAR)\s+PTR\s+\?", "?", block)


def _nested(body):
    """The struct types a block's fields are declared with."""
    return [t for t in re.findall(r"^\w+\s+([\w$]+)\s", body, re.M)
            if t.upper() not in ("DB", "DW", "DD", "DF", "DQ", "DT")]


def defines(prefix):
    """An `EQU` for every numeric `#define` whose name starts with `prefix`,
    read from the headers directly. **Not H2ASH's**: it misreads a padded
    define's value - `#define SC_TAB    0x0f` comes out `00h`, and `10`
    as `1` - and a number needs no compiler to read."""
    found = []
    for path in sorted(set(_headers().values())):
        found += re.findall(r"^#define\s+(%s\w*)\s+(0x[0-9a-fA-F]+|\d+)\b"
                            % re.escape(prefix), open(path).read(), re.M)
    if not found:
        raise SystemExit("h2ash: no #define starting %s in the headers" % prefix)
    return ["%s EQU %d" % (n, int(v, 0)) for n, v in found]


def structs_ash(wanted):
    """The include for `wanted`, a list of (struct, alias) pairs; a struct
    written `PREFIX*` is the `#define`s of that prefix instead. A struct
    a field is declared with comes first, without constants of its own
    unless it was asked for too; an anonymous one - H2ASH's `tag$N` - is
    refused, because its name is H2ASH's and would move: name it in C."""
    text = h2ash_output()
    blocks, consts, done = [], [], set()
    equs = [e for struct, _ in wanted if struct.endswith("*")
            for e in defines(struct[:-1])]
    wanted = [(s, a) for s, a in wanted if not s.endswith("*")]

    def emit(struct):
        if struct in done:
            return
        if "$" in struct:
            raise SystemExit("h2ash: an anonymous struct (%s) - give it a "
                             "name in the header" % struct)
        m = _block(text, struct)
        for inner in _nested(m.group(1)):
            emit(inner)
        done.add(struct)
        if m.group(0).startswith("UNION"):
            body = _union_body(m.group(0))
        else:
            body = m.group(0)
        blocks.append(_struct_arrays(struct, body))

    for struct, _ in wanted:
        emit(struct)
    # Constants for every struct in the include: the ones asked for under
    # their aliases, the ones they nest under their own names - an offset
    # into a nested struct is written `outer_field+inner_field`.
    aliases = dict(wanted)
    for struct in [b.split()[1] for b in blocks]:
        alias = aliases.get(struct, struct)
        m = _block(text, struct)
        for field in re.findall(r"^(\w+)\s+\S", m.group(1), re.M):
            name = "%s_%s" % (alias, field)
            if re.search(r"^(?:STRUC|UNION)\s+%s\s*$" % name, text, re.M):
                raise SystemExit("h2ash: the constant %s would be the struct "
                                 "of that name - choose another alias for %s"
                                 % (name, struct))
            consts.append("%s = OFFSET (%s PTR 0).%s" % (name, struct, field))
    return ("; Generated by tools/h2ash.py from the port's headers.\n"
            "IDEAL\n\n" + "\n\n".join(blocks) + "\n\n" + "\n".join(consts)
            + "\n\n" + "\n".join(equs) + "\n\nMASM\n")


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
