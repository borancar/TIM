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
# The part kinds' records: in DGROUP in 1.00, in a far segment of their own
# in 1.11 (tools/kindtables.py). 1.00 has 58 kinds; 1.11 keeps them first.
OLD_KIND_TABLE, NEW_KIND_TABLE, OLD_KINDS = OLD_DGROUP + 0x0EA6, 0x2EF10, 58
# The part templates, 16 bytes a kind, ending in the init routine's far
# pointer: DGROUP 0x2966 in 1.00, 0x2488 in 1.11.
OLD_TEMPLATES, NEW_TEMPLATES = OLD_DGROUP + 0x2966, NEW_DGROUP + 0x2488

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


def provenance_of(root):
    """Every routine's (file, name) -> address, as the sources under `root`
    give them."""
    import provenance
    out = {}
    for p in sorted(glob.glob(os.path.join(root, "src", "*.c"))
                    + glob.glob(os.path.join(root, "src", "parts", "*.c"))):
        rel = os.path.relpath(p, root)
        if os.path.basename(p).startswith(OVERLAYS):
            continue
        t, ours, stubs, bare, internal, errs = provenance.check(p)
        for name, a in t + stubs:
            if a != "?":
                out[rel, name] = int(a, 16)
    return out


def routines():
    """1.00's routines at 1.00's addresses - read from the sources as tagged
    `tim-1.00` (exported to out/tim-1.00/src), since the tree's own have
    moved - each with the address the tree gives it now, `cur`, which is
    what `--apply` rewrites."""
    old_root = os.path.join(OLD_DIR, "src", "reconstruct")
    if not os.path.isdir(old_root):
        raise SystemExit("export the tag first: git archive tim-1.00 reconstruct "
                         "| tar -x -C out/tim-1.00/src")
    cur = provenance_of(os.path.join(REPO, "reconstruct"))
    rows = []
    for (rel, name), a in provenance_of(old_root).items():
        rows.append(dict(file=rel, name=name, old=a, cur=cur.get((rel, name))))
    rows.sort(key=lambda r: r["old"])
    for i, r in enumerate(rows):
        nxt = rows[i + 1]["old"] if i + 1 < len(rows) else OLD_DGROUP
        r["size"] = max(0, min(nxt, OLD_DGROUP) - r["old"])
    return rows


def entries(img, exe):
    """**Where 1.11's routines start**, as the image itself says: every far
    call's target (a `9a` whose segment word is relocated), every near call's
    target within its caller's segment, and every relocated far pointer whose
    offset lands on a `push bp / mov bp, sp` - the handler tables. A routine
    placed anywhere else is placed mid-routine."""
    rel = relocs(exe)
    frames = frames_111()
    out = set()
    for r in rel:
        if r < 3 or r + 2 > len(img):
            continue
        seg = struct.unpack_from("<H", img, r)[0]
        off = struct.unpack_from("<H", img, r - 2)[0]
        at = seg * 16 + off
        if at >= NEW_DGROUP:
            continue
        if img[r - 3] == 0x9A and r < NEW_DGROUP:
            out.add(at)
        elif img[at:at + 3] == b"\x55\x8b\xec":
            out.add(at)
    for i in range(len(img) - 3):
        if i >= NEW_DGROUP:
            break
        if img[i] == 0xE8 and i and img[i - 1] == 0x0E:
            f = max(x for x in frames if x <= i) if frames else 0
            t = (i + 3 - f + struct.unpack_from("<h", img, i + 1)[0]) & 0xFFFF
            if img[f + t:f + t + 1] == b"\x55" or img[f + t:f + t + 2] in (b"\x56\x57",):
                out.add(f + t)
    # A prologue straight after a return: a routine nothing calls directly,
    # or calls in a form not read above.
    p = img.find(b"\x55\x8b\xec")
    while p != -1 and p < NEW_DGROUP:
        if img[p - 1] in (0xCB, 0xC3) or img[p - 3] in (0xCA, 0xC2):
            out.add(p)
        p = img.find(b"\x55\x8b\xec", p + 1)
    return out


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
    starts = entries(new, NEW_EXE)
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
            if have[k][0] not in starts:
                continue
            ratio = difflib.SequenceMatcher(
                None, want, seq[k:k + len(want) + len(want) // 4],
                autojunk=False).ratio()
            if ratio > best[0]:
                best = (ratio, have[k][0])
        if best[1] is not None and best[0] >= 0.4:
            r["new"], r["how"], r["ratio"] = best[1], "resembles", round(best[0], 3)
            cursor = best[1] + 1
        r.pop("candidates", None)
    for r in rows:
        r.pop("candidates", None)

    # **The third pass, out of order**: a module 1.11 links somewhere else -
    # the sound library, whose routines each became a segment of their own
    # in another order - has no window between placed neighbours to be
    # found in. What is still unplaced is compared with every entry of 1.11's
    # that nothing has claimed, best pairs first, and a pair is taken only
    # when it is clearly the best for the routine (0.75 or better, and 0.15
    # ahead of its next choice).
    taken = {r["new"] for r in rows if "new" in r}
    free = [e for e in sorted(starts) if e not in taken]
    ends = sorted(starts) + [NEW_DGROUP]
    def extent(e):
        return min(ends[bisect.bisect_right(ends, e)] - e, 0x1000)
    have = {e: [t for a, t in tokens(new, e, extent(e))] for e in free}
    pairs = []
    for r in rows:
        if "new" in r or r["size"] < 8:
            continue
        want = [t for a, t in tokens(old, r["old"], r["size"])]
        if len(want) < 3:
            continue
        scores = []
        for e in free:
            h = have[e]
            if not h or not 0.5 < len(h) / len(want) < 2:
                continue
            m = difflib.SequenceMatcher(None, want, h, autojunk=False)
            if m.real_quick_ratio() < 0.75 or m.quick_ratio() < 0.75:
                continue
            scores.append((m.ratio(), e))
        scores.sort(reverse=True)
        if scores and scores[0][0] >= 0.75 and (
                len(scores) == 1 or scores[0][0] - scores[1][0] >= 0.15):
            pairs.append((scores[0][0], scores[0][1], r))
    pairs.sort(key=lambda p: -p[0])
    for ratio, e, r in pairs:
        if e in taken or "new" in r:
            continue
        r["new"], r["how"], r["ratio"] = e, "resembles, moved", round(ratio, 3)
        taken.add(e)

    # **The kind table outranks all of it.** Each kind's record holds six far
    # pointers to its handlers, in both builds, so a 1.00 handler and its
    # 1.11 counterpart are the same slot of the same kind's record: an exact
    # pairing, where resemblance guessed - and got some wrong, as the
    # handlers of a kind are short and alike. Where slots disagree the
    # clear majority wins: kinds 51..54 had the shared do-nothing handlers
    # in 1.00 and have routines of their own in 1.11, and every other kind
    # still names the shared ones.
    by_old = {r["old"]: r for r in rows}
    seen = {}
    for k in range(OLD_KINDS):
        for i in range(6):
            o, sg = struct.unpack_from("<HH", old, OLD_KIND_TABLE + k * 0x3A + 0x22 + 4 * i)
            o2, sg2 = struct.unpack_from("<HH", new, NEW_KIND_TABLE + k * 0x3A + 0x22 + 4 * i)
            seen.setdefault(sg * 16 + o, []).append(sg2 * 16 + o2)
        o, sg = struct.unpack_from("<HH", old, OLD_TEMPLATES + 16 * k + 12)
        o2, sg2 = struct.unpack_from("<HH", new, NEW_TEMPLATES + 16 * k + 12)
        if o or sg:
            seen.setdefault(sg * 16 + o, []).append(sg2 * 16 + o2)
    for a, bs in seen.items():
        r = by_old.get(a)
        if r is None:
            continue
        import collections
        b, votes = collections.Counter(bs).most_common(1)[0]
        if votes * 2 <= len(bs):
            continue
        if r.get("new") != b:
            r["was"] = r.get("new")
        r["new"], r["how"] = b, "kind table"
        r.pop("ratio", None)

    for r in rows:
        if "new" in r:
            r["entry"] = r["new"] in starts
    return rows


SURE = ("exact", "exact in order", "kind table")
# A resemblance is only ever at a routine's entry (`entries`), so it is a
# choice between whole routines, and the one it picks at 0.6 has been the
# routine the callers call wherever that was checked.
STRONG = 0.6


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
            if r.get("cur") is None:
                continue
            old = "0x%05x" % r["cur"]
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
                    body = m.group(5).replace(" (1.00's; not yet placed in 1.11)", "")
                    return m.group(1) + at + body
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
    # tim.h: `name(...);   /* 0x..... */` or `/* seg:off, 0x..... */`,
    # rewritten by name to the settled address whatever it says now - the
    # prototypes are copies, and a copy can hold an address the source no
    # longer does.
    th = os.path.join(REPO, "reconstruct", "tim.h")
    text = open(th).read()
    out = text
    for r in rows:
        if not settled(r):
            continue
        def rep(m, r=r):
            at = "0x%05x" % r["new"]
            if m.group(2):
                f = max(x for x in fr if x <= r["new"])
                at = "%04x:%04x, %s" % (f >> 4, r["new"] - f, at)
            return m.group(1) + at
        out = re.sub(r"(\b" + re.escape(r["name"]) + r"\s*\([^;]*\);[ \t]*/\* *)"
                     r"(?:([0-9a-f]{4}):([0-9a-f]{4}), )?0x[0-9a-f]{5}\b", rep, out)
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
