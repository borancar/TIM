"""Where each of 1.00's DGROUP objects is in 1.11: the data half of the move.

`tools/addrmap.py` pairs each of 1.00's routines with 1.11's. A paired
routine names the same data, so wherever the two line up instruction for
instruction, a DGROUP operand in 1.00's is the same object's in 1.11's. This
collects those pairs from every routine the map placed, and each 1.00 public
object - from 1.00's link map, `out/v1.00/link/exe/TIM.MAP`, from the last
`TIM_VERSION=1.00` link - takes the 1.11 address most of its references
agree on: a reference `k` bytes into the object votes for `new - k`.

The instructions are lined up with difflib over their normalised forms
(`addrmap.norm`), and only the runs that are equal pair their operands: a
direct memory operand, or one indexed off BX, SI or DI with no segment
override - the operand forms whose displacement is a DGROUP address. A pushed
or moved immediate can be an address too (a string's), but is as often a
constant, so immediates only vote where no memory operand did, and are
reported as such.

Each object says how many references voted and how many agreed, and
`--report` lists those that did not agree or had no vote. The map is written
to `out/datamap.json`.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import bisect
import collections
import difflib
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import capstone
import addrmap as A

OLD_MAP = os.path.join(A.OLD_DIR, "link", "exe", "TIM.MAP")
OUT = os.path.join(A.NEW_DIR, "datamap.json")
OLD_DATA_END = 0x6500          # past the last 1.00 object the map names
NEW_DATA_END = 0x6500
X = capstone.x86


def publics():
    out = {}
    for m in re.finditer(r"^ 2D3C:([0-9A-F]{4})(?: idle)?\s+_(\w+)\s*$",
                         open(OLD_MAP, errors="replace").read(), re.M):
        out.setdefault(m.group(2), int(m.group(1), 16))
    return sorted(((a, n) for n, a in out.items()))


def instructions(img, a, n):
    return list(A.MD.disasm(img[a:a + n], a))


def data_operands(ins):
    """(kind, value) for each operand that may be a DGROUP address."""
    out = []
    for op in ins.operands:
        if op.type == X.X86_OP_MEM:
            m = op.mem
            if m.segment not in (0, X.X86_REG_DS):
                continue
            if m.base in (X.X86_REG_BP, X.X86_REG_SP) or m.index == X.X86_REG_BP:
                continue
            if ins.mnemonic in ("lcall", "ljmp") or ins.mnemonic.startswith("les") \
                    or ins.mnemonic.startswith("lds"):
                if m.base or m.index:
                    continue
            out.append(("mem", m.disp & 0xFFFF))
        elif op.type == X.X86_OP_IMM and ins.mnemonic in ("mov", "push"):
            out.append(("imm", op.imm & 0xFFFF))
        else:
            out.append((None, None))
    return out


def pairs(old, new, r, ends):
    """(kind, old address, new address) for every operand pair the two
    routines line up on."""
    ext = min(ends[bisect.bisect_right(ends, r["new"])] - r["new"], r["size"] * 2 + 64)
    a = instructions(old, r["old"], r["size"])
    b = instructions(new, r["new"], ext)
    sm = difflib.SequenceMatcher(None, [A.norm(i) for i in a], [A.norm(i) for i in b],
                                 autojunk=False)
    out = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != "equal":
            continue
        for x, y in zip(a[i1:i2], b[j1:j2]):
            for (k1, v1), (k2, v2) in zip(data_operands(x), data_operands(y)):
                if k1 and k1 == k2 and v1 < OLD_DATA_END and v2 < NEW_DATA_END:
                    # An immediate that did not change is as likely a
                    # constant as an address that did not move.
                    if k1 == "imm" and v1 == v2:
                        continue
                    out.append((k1, v1, v2))
    return out


def build():
    old, new = open(A.OLD_IMG, "rb").read(), open(A.NEW_IMG, "rb").read()
    rows = json.load(open(A.OUT))
    ends = sorted(A.entries(new, A.NEW_EXE)) + [A.NEW_DGROUP]
    objs = publics()
    starts = [a for a, n in objs]
    votes = collections.defaultdict(lambda: {"mem": collections.Counter(),
                                             "imm": collections.Counter()})
    for r in rows:
        if "new" not in r or r["size"] < 4:
            continue
        if r["how"] not in A.SURE and r.get("ratio", 0) < 0.7:
            continue
        for kind, o, n in pairs(old, new, r, ends):
            i = bisect.bisect_right(starts, o) - 1
            if i < 0:
                continue
            base, name = objs[i]
            votes[name][kind][n - (o - base)] += 1
    out = []
    for base, name in objs:
        v = votes.get(name)
        row = dict(name=name, old=base)
        if i := bisect.bisect_right(starts, base):
            row["size"] = starts[i] - base if i < len(starts) else None
        for kind in ("mem", "imm"):
            if v and v[kind]:
                at, agree = v[kind].most_common(1)[0]
                row.update(new=at, how=kind, agree=agree, votes=sum(v[kind].values()))
                break
        out.append(row)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()
    rows = build()
    json.dump(rows, open(OUT, "w"), indent=1)
    c = collections.Counter(r.get("how", "none") for r in rows)
    split = [r for r in rows if "new" in r and r["agree"] < r["votes"]]
    print("%d objects: %s; %d with votes that disagree"
          % (len(rows), ", ".join("%d %s" % (v, k) for k, v in c.items()), len(split)))
    if args.report:
        for r in rows:
            if "new" not in r or r["agree"] < r["votes"] or r["how"] == "imm":
                print("  %-36s %04x %5s  %s" % (
                    r["name"], r["old"], r.get("size"),
                    "-" if "new" not in r else "-> %04x %s %d/%d" % (
                        r["new"], r["how"], r["agree"], r["votes"])))
    print("written", OUT)


if __name__ == "__main__":
    main()
