"""Where each of 1.00's routines is in 1.11: the address map the move needs.

Every transcribed routine carries the 1.00 image address it came from, and the
judge finds a routine in the image by it. The target is now 1.11, where the
same routines sit elsewhere, so this measures, for each one, where 1.11 has
it - and how sure that answer is.

The routines are the port's own, read the way `reconstruct/tests/provenance.py`
reads them: the address on the first line of the comment above a definition.
The overlay files are left out; their addresses are offsets into VM.OVL and
SX.OVL, which 1.11 carries as binaries of its own. A routine's extent in 1.00
runs to the next routine's address.

A routine is placed in two passes:

  1. **exactly**: its bytes, with every immediate, displacement, near and far
     call target and relocated word masked - the things that move with the
     layout - found in 1.11's image. One hit is a placement; several are kept
     as candidates for the second pass to choose between;
  2. **by resemblance**, for the rest: its instructions, normalised to the
     mnemonic and register operands, against 1.11's between the placements of
     the routines on either side of it. Link order does not change between the
     two builds, so that window is where it has to be.

Each placement says which pass made it and, for the second, how alike the two
are (difflib's ratio over the normalised instructions). The map is written to
`out/addrmap.json`; `--report` prints what is weak or missing.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import bisect
import difflib
import glob
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "reconstruct", "tests"))

import capstone

OLD_DIR = os.path.join(REPO, "out", "tim-1.00")
OLD_IMG = os.path.join(OLD_DIR, "TIM.img")
OLD_EXE = os.path.join(OLD_DIR, "TIM.unpacked.exe")
NEW_IMG = os.path.join(REPO, "out", "TIM.img")
NEW_EXE = os.path.join(REPO, "out", "TIM.unpacked.exe")
OUT = os.path.join(REPO, "out", "addrmap.json")
OVERLAYS = ("sxovl", "vmovl")
OLD_DGROUP, NEW_DGROUP = 0x2D3C0, 0x2FE10

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
MD.detail = True


def relocs(exe):
    d = open(exe, "rb").read()
    n, tbl = struct.unpack_from("<H", d, 6)[0], struct.unpack_from("<H", d, 0x18)[0]
    out = set()
    for i in range(n):
        o, s = struct.unpack_from("<HH", d, tbl + 4 * i)
        out.add(s * 16 + o)
    return out


def routines():
    import provenance
    rows = []
    for p in sorted(glob.glob(os.path.join(REPO, "reconstruct", "src", "*.c"))
                    + glob.glob(os.path.join(REPO, "reconstruct", "src", "parts", "*.c"))):
        rel = os.path.relpath(p, os.path.join(REPO, "reconstruct"))
        if os.path.basename(p).startswith(OVERLAYS):
            continue
        t, ours, stubs, bare, internal, errs = provenance.check(p)
        for name, a in t + stubs:
            if a != "?":
                rows.append(dict(file=rel, name=name, old=int(a, 16)))
    rows.sort(key=lambda r: r["old"])
    for i, r in enumerate(rows):
        nxt = rows[i + 1]["old"] if i + 1 < len(rows) else OLD_DGROUP
        r["size"] = max(0, min(nxt, OLD_DGROUP) - r["old"])
    return rows


def pattern(img, rel, a, n):
    buf = img[a:a + n]
    mask = bytearray(b"\x01" * len(buf))
    for ins in MD.disasm(buf, a):
        o = ins.address - a
        if ins.imm_offset:
            for k in range(ins.imm_offset, ins.size):
                mask[o + k] = 0
        if ins.disp_offset:
            for k in range(ins.disp_offset, ins.disp_offset + (ins.disp_size or 2)):
                if o + k < len(mask):
                    mask[o + k] = 0
        if buf[o] in (0xE8, 0xE9, 0x9A, 0xEA):
            for k in range(1, ins.size):
                mask[o + k] = 0
    for r in rel:
        if a <= r < a + n:
            mask[r - a] = 0
            if r - a + 1 < n:
                mask[r - a + 1] = 0
    return buf, mask


def exact_hits(new, buf, mask, lo_hi=None):
    best, i = (0, 0), 0
    while i < len(buf):
        if mask[i]:
            j = i
            while j < len(buf) and mask[j]:
                j += 1
            if j - i > best[1] - best[0]:
                best = (i, j)
            i = j
        else:
            i += 1
    lo, hi = best
    if hi - lo < 3:
        return None
    anchor, hits = buf[lo:hi], []
    p = new.find(anchor)
    while p != -1:
        s = p - lo
        if 0 <= s and s + len(buf) <= NEW_DGROUP and all(
                not mask[k] or new[s + k] == buf[k] for k in range(len(buf))):
            hits.append(s)
        p = new.find(anchor, p + 1)
    return hits


def norm(ins):
    ops = []
    for op in ins.operands:
        if op.type == capstone.x86.X86_OP_REG:
            ops.append(ins.reg_name(op.reg))
        elif op.type == capstone.x86.X86_OP_IMM:
            ops.append("i")
        else:
            m = op.mem
            ops.append("[%s%s]" % (ins.reg_name(m.base) if m.base else "",
                                   "+" + ins.reg_name(m.index) if m.index else ""))
    return ins.mnemonic + " " + ",".join(ops)


def tokens(img, a, n):
    return [(ins.address, norm(ins)) for ins in MD.disasm(img[a:a + n], a)]


def build():
    old, new = open(OLD_IMG, "rb").read(), open(NEW_IMG, "rb").read()
    orel = relocs(OLD_EXE)
    rows = routines()
    for r in rows:
        if r["size"] < 4:
            r["how"] = "too short"
            continue
        buf, mask = pattern(old, orel, r["old"], r["size"])
        hits = exact_hits(new, buf, mask)
        if hits is None:
            r["how"] = "too short"
        elif len(hits) == 1:
            r["new"], r["how"] = hits[0], "exact"
            r["same_bytes"] = new[hits[0]:hits[0] + r["size"]] == buf
        elif hits:
            r["candidates"], r["how"] = hits, "ambiguous"
        else:
            r["how"] = "unplaced"

    # **Two routines placed at one address.** With the constants masked, two
    # small wrappers that differ only in an argument look the same, and if
    # 1.11 kept one of them both patterns find it. The one whose bytes agree
    # best, masked parts included, keeps the address; the other goes to the
    # second pass.
    by_new = {}
    for r in rows:
        if "new" in r:
            by_new.setdefault(r["new"], []).append(r)
    for at, rs in by_new.items():
        if len(rs) < 2:
            continue
        def agree(r):
            a = old[r["old"]:r["old"] + r["size"]]
            return sum(x == y for x, y in zip(a, new[at:at + r["size"]]))
        rs.sort(key=agree, reverse=True)
        for r in rs[1:]:
            del r["new"]
            r.pop("same_bytes", None)
            r["how"] = "unplaced"

    # The second pass, in link order: each unplaced routine searched for
    # between its placed neighbours, which also settles the ambiguous ones.
    # A placement made here moves the lower bound for the next: routines keep
    # their order, so a run of look-alikes - the part hooks that all answer
    # 0, the goal tests that all return - lands one after another instead of
    # all on the first of them.
    placed = [(r["old"], r["new"]) for r in rows if "new" in r]
    olds = [p[0] for p in placed]
    cursor = 0
    for r in rows:
        if "new" in r:
            cursor = max(cursor, r["new"] + 1)
            continue
        i = bisect.bisect_left(olds, r["old"])
        lo = max(placed[i - 1][1] + 1 if i > 0 else 0, cursor)
        hi = placed[i][1] if i < len(placed) else NEW_DGROUP
        if hi <= lo:
            continue
        if r.get("candidates"):
            inside = [c for c in r["candidates"] if lo <= c < hi]
            if inside:
                r["new"], r["how"] = inside[0], "exact in order"
                cursor = inside[0] + 1
                continue
        if r["size"] < 4 or hi - lo > 0x4000:
            continue
        want = [t for a, t in tokens(old, r["old"], r["size"])]
        if len(want) < 3:
            continue
        have = tokens(new, lo, hi - lo)
        seq = [t for a, t in have]
        best = (0.0, None)
        for k in range(len(have)):
            if have[k][1].split()[0] != want[0].split()[0]:
                continue
            ratio = difflib.SequenceMatcher(
                None, want, seq[k:k + len(want) + len(want) // 4],
                autojunk=False).ratio()
            if ratio > best[0]:
                best = (ratio, have[k][0])
        if best[1] is not None and best[0] >= 0.5:
            r["new"], r["how"], r["ratio"] = best[1], "resembles", round(best[0], 3)
            cursor = best[1] + 1
        r.pop("candidates", None)
    for r in rows:
        r.pop("candidates", None)
    return rows


SURE = ("exact", "exact in order")
STRONG = 0.8


def settled(r):
    """A placement good enough to move a routine's address to."""
    return "new" in r and (r["how"] in SURE or r.get("ratio", 0) >= STRONG)


def frames_111():
    """1.11's segment frames, as the judge measures them: every far call's
    target segment."""
    exe = open(NEW_EXE, "rb").read()
    n, hdr, tbl = (struct.unpack_from("<H", exe, 6)[0],
                   struct.unpack_from("<H", exe, 8)[0] * 16,
                   struct.unpack_from("<H", exe, 0x18)[0])
    img = exe[hdr:]
    seen = {0}
    for i in range(n):
        off, seg = struct.unpack_from("<HH", exe, tbl + 4 * i)
        at = seg * 16 + off
        if at >= 3 and img[at - 3] == 0x9A:
            seen.add(struct.unpack_from("<H", img, at)[0] * 16)
    return sorted(seen)


def apply(rows, dry):
    """Move each settled routine's provenance to its 1.11 address, where the
    judge reads it: the first line of the comment above a definition (`0x...`
    or `seg:off, image 0x...`), the comment above an assembly module's
    `proc`, and a Borland-only routine's `/* 0x... */` - and tim.h's
    prototype comments. A routine that is not settled keeps its 1.00 address
    and says so on the same line, so the judge still finds a number and a
    reader sees it is not 1.11's."""
    import re
    fr = frames_111()
    by_file = {}
    for r in rows:
        by_file.setdefault(r["file"], {})[r["name"]] = r
    def new_text(r, old_text):
        """`old_text` is the address as the source spells it."""
        if not settled(r):
            return None
        return "0x%05x" % r["new"]
    changed = moved = marked = 0
    for rel, names in sorted(by_file.items()):
        path = os.path.join(REPO, "reconstruct", rel)
        text = open(path).read()
        out = text
        for name, r in names.items():
            old = "0x%05x" % r["old"]
            olds = [old, old.upper().replace("0X", "0x")]
            # 1. the comment above the C definition, and 2./3. the TASM and
            # Borland-only forms: any of them is a comment ending right
            # before the routine's name is defined.
            pat = re.compile(
                r"(/\*(?:(?!\*/).)*?)(?:([0-9a-f]{4}):([0-9a-f]{4}),\s*image\s+)?"
                + r"(" + "|".join(re.escape(o) for o in olds) + r")\b"
                + r"((?:(?!\*/).)*?\*/\s*\n"
                + r"(?:[^\n]*\n){0,1}?[^\n]*?\b_?" + re.escape(name) + r"\b)",
                re.S)
            def rep(m):
                nonlocal moved, marked
                if settled(r):
                    moved += 1
                    at = "0x%05x" % r["new"]
                    if m.group(2):
                        f = max(x for x in fr if x <= r["new"])
                        at = "%04x:%04x, image %s" % (f >> 4, r["new"] - f, at)
                    return m.group(1) + at + m.group(5)
                marked += 1
                note = " (1.00's; not yet placed in 1.11)"
                body = m.group(5)
                if note.strip() in body:
                    return m.group(0)
                return (m.group(1) + (("%s:%s, image " % (m.group(2), m.group(3)))
                                      if m.group(2) else "")
                        + m.group(4) + note + body)
            out = pat.sub(rep, out)
        if out != text:
            changed += 1
            if not dry:
                open(path, "w").write(out)
    # tim.h: `name(...);   /* 0x..... */`
    th = os.path.join(REPO, "reconstruct", "tim.h")
    text = open(th).read()
    out = text
    for r in rows:
        if not settled(r):
            continue
        out = re.sub(r"(\b" + re.escape(r["name"]) + r"\s*\([^;]*\);[ \t]*/\* *)0x%05x\b" % r["old"],
                     lambda m: m.group(1) + "0x%05x" % r["new"], out)
    if out != text:
        changed += 1
        if not dry:
            open(th, "w").write(out)
    print("%s %d files: %d addresses moved to 1.11, %d kept as 1.00's and marked"
          % ("would change" if dry else "changed", changed, moved, marked))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", action="store_true",
                    help="list the weak and missing placements")
    ap.add_argument("--apply", action="store_true",
                    help="rewrite the sources' provenance addresses from the map")
    ap.add_argument("--dry-run", action="store_true",
                    help="with --apply: say what would change, write nothing")
    args = ap.parse_args()
    rows = build()
    if args.apply:
        apply(rows, args.dry_run)
        return
    json.dump(rows, open(OUT, "w"), indent=1)
    from collections import Counter
    c = Counter(r["how"] for r in rows)
    same = sum(1 for r in rows if r.get("same_bytes"))
    print("%d routines: %s; %d of the exact ones byte for byte"
          % (len(rows), ", ".join("%d %s" % (v, k) for k, v in c.most_common()), same))
    # Link order is the same in both builds, so a placement out of order is a
    # wrong one - or a module that moved, which is worth knowing either way.
    placed = [r for r in rows if "new" in r]
    for a, b in zip(placed, placed[1:]):
        if b["new"] <= a["new"]:
            print("  out of order: %s %05x->%05x (%s), then %s %05x->%05x (%s)"
                  % (a["name"], a["old"], a["new"], a["how"],
                     b["name"], b["old"], b["new"], b["how"]))
    if args.report:
        for r in rows:
            if r["how"] not in ("exact", "exact in order") and not (
                    r["how"] == "resembles" and r["ratio"] >= 0.8):
                print("  %-26s %-34s %05x %5d  %s %s" % (
                    r["file"], r["name"], r["old"], r["size"], r["how"],
                    "" if "new" not in r else "-> %05x (%.2f)" % (r["new"], r.get("ratio", 1))))
    print("written", OUT)


if __name__ == "__main__":
    main()
