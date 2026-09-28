#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""**Links TIM.EXE from the reconstruction** and says how far it is from the
original.

Every game module in `reconstruct/src` is built exactly as the judge builds
it - `judge.py` itself runs on each file with `JUDGE_KEEP_OBJ`, so the
compiler, options and assembler are the file's own markers - and the objects
go to Borland C++ 3.0's TLINK, under turboc's emulator, in image order behind
the 3.0 start-up `C0M.OBJ` and against `CM.LIB`, the run-time library the
image was measured to hold (STATUS.md). The result is compared with the
unpacked original, `out/TIM.unpacked.exe`: its header, its load module byte
for byte, and its relocations as a set.

    uv run python tools/link.py            # build, link, compare
    uv run python tools/link.py --reuse    # link the objects of the last run

What is not a judged module is not here: `dgroup.c` is the host's layout of
DGROUP and does not compile under Borland, so data no module owns yet is
missing from the link, and the comparison says where.

This file is the port's own tooling, not a transcription.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
TURBOC = os.path.join(REPO, "..", "turboc")
LIB = os.path.join(TURBOC, "dos-c", "BCPP300", "LIB")
OUT = os.path.join(REPO, "out", "link")
ORIGINAL = os.path.join(REPO, "out", "TIM.unpacked.exe")

sys.path.insert(0, HERE)
import judge  # noqa: E402

# not game code in TIM.EXE: the host's DGROUP, the entry point we wrote,
# the two overlays (separate binaries)
NOT_LINKED = re.compile(r"/(dgroup|main|sxovl\w*|vmovl\w*)\.c$")


def game_files():
    return [f for f in judge.port_sources()
            if "/reconstruct/src/" in f and f.endswith(".c")
            and not NOT_LINKED.search(f)]


def first_address(path):
    addrs = judge.addresses([path]).values()
    return min(addrs) if addrs else None


DATA_AT = re.compile(r"\[_DATA\]\s+MATCH, at DGROUP ([0-9a-f]{4})")


def build(path, obj):
    """The judge on one file, keeping the object it built. Answers the
    verdict and where the judge placed the module's `_DATA`, if it has
    any."""
    r = subprocess.run([sys.executable, os.path.join(HERE, "judge.py"), path],
                       cwd=REPO, capture_output=True, text=True,
                       env=dict(os.environ, JUDGE_KEEP_OBJ=obj))
    verdict = [l for l in r.stdout.splitlines() if "routines match" in l]
    m = DATA_AT.search(r.stdout)
    return (path, os.path.exists(obj), verdict[-1] if verdict else r.stdout[-300:],
            int(m.group(1), 16) if m else None)


def object_order(mods, fr):
    """**The order the objects were linked in**, which is not the code's:
    TLINK lays each segment out in object order, and the image's DGROUP has
    `gamemain.c`'s data first and `collide.c`'s - the first module of
    segment 0000 - well after. So a module with data goes where its data is,
    and one without follows the module before it in its own code segment
    (or leads the one after it). `mods` is (path, code address or None,
    data address or None)."""
    key = {}
    for path, code, data in mods:
        if data is not None:
            key[path] = float(data)
    by_seg = {}
    for path, code, data in mods:
        if code is not None:
            by_seg.setdefault(judge.frame_of(code, fr), []).append((code, path))
    for seg in by_seg.values():
        seg.sort()
        for k, (code, path) in enumerate(seg):
            if path in key:
                continue
            prev = [key[p] for _c, p in seg[:k] if p in key]
            nxt = [key[p] for _c, p in seg[k + 1:] if p in key]
            key[path] = (prev[-1] + 0.001 * (k + 1)) if prev else \
                (nxt[0] - 0.001 * (len(seg) - k)) if nxt else 1e9 + code
    return sorted(mods, key=lambda m: (key[m[0]], m[1] or 0))


def _rec(t, payload):
    body = bytes([t]) + struct.pack("<H", len(payload) + 1) + bytes(payload)
    return body + bytes([(-sum(body)) & 0xFF])


def one_code_segment(obj, frame):
    """**The modules of one physical segment share one code segment.** Each
    object names its own (`LZHUF_TEXT`, `BITMAPS_TEXT`), and TLINK then
    keeps them apart, so a near call between two of them - which the image
    has - overflows. The name is not in the EXE, so it is ours: the frame's
    paragraph, `S1C25_TEXT` - but `_TEXT` for segment 0000, the start-up's
    and the library's too."""
    out = []
    for t, p, _at in judge.omf.records(open(obj, "rb").read()):
        if t == 0x96:
            q, i = b"", 0
            while i < len(p):
                n = p[i + 1:i + 1 + p[i]].decode("cp437")
                if n.endswith("_TEXT"):
                    # segment 0000 is the start-up's and the library's
                    # `_TEXT`, which TLINK fills in that order after C0M
                    n = "_TEXT" if frame == 0 else "S%04X_TEXT" % (frame >> 4)
                q += bytes([len(n)]) + n.encode("cp437")
                i += 1 + p[i]
            p = q
        out.append(_rec(t, p))
    open(obj, "wb").write(b"".join(out))


def link(objs):
    """TLINK /c /m /s under the emulator: C0M first, the modules in image
    order, CM.LIB. Answers the linker's output."""
    names = ["C0M.OBJ"] + [os.path.basename(o) for o in objs]
    lines, cur = [], ""
    for n in names:
        if len(cur) + len(n) + 3 > 100:
            lines.append(cur + " +")
            cur = ""
        cur += (" " if cur else "") + n
    lines.append(cur)
    lines += ["TIM", "TIM", "CM.LIB"]
    rsp = os.path.join(OUT, "TIM.RSP")
    open(rsp, "w", newline="").write("\r\n".join(lines) + "\r\n")
    save = os.path.join(OUT, "exe")
    os.makedirs(save, exist_ok=True)
    cmd = ["uv", "run", "--project", TURBOC, "python",
           os.path.join(HERE, "tcrun.py"), "TLINK.EXE", "--save", save,
           "--add", rsp, "--add", os.path.join(LIB, "C0M.OBJ"),
           "--add", os.path.join(LIB, "CM.LIB")]
    for o in objs:
        cmd += ["--add", o]
    cmd += ["--", "/c", "/m", "/s", "@TIM.RSP"]
    r = subprocess.run(cmd, cwd=TURBOC, capture_output=True, text=True,
                       env=dict(os.environ, TURBOC_VERSION="bc3.00"))
    return r.stdout + r.stderr, os.path.join(save, "TIM.EXE")


def mz(data):
    """(header fields, load module, relocations as a set of image offsets)."""
    (sig, last, pages, nrel, hdr, minalloc, maxalloc, ss, sp, _csum, ip, cs,
     reloff) = struct.unpack_from("<HHHHHHHHHHHHH", data, 0)
    size = pages * 512 - (512 - last if last else 0)
    body = data[hdr * 16:size]
    rel = set()
    for k in range(nrel):
        o, s = struct.unpack_from("<HH", data, reloff + 4 * k)
        rel.add(s * 16 + o)
    fields = {"minalloc": minalloc, "maxalloc": maxalloc, "ss": ss, "sp": sp,
              "cs": cs, "ip": ip, "relocations": nrel, "size": len(body)}
    return fields, body, rel


def runs(a, b):
    """The differing byte runs of two equal-length stretches, as (start,
    end) pairs."""
    out, start = [], None
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            if start is None:
                start = i
        elif start is not None:
            out.append((start, i))
            start = None
    if start is not None:
        out.append((start, min(len(a), len(b))))
    return out


# **Where the program ends in the recovered image**: the stack's end, DGROUP
# 0x6550. Above it the image unlzexe.py read out of memory holds what LZEXE's
# stub left there - its own copy and the tail of the compressed data, found
# verbatim in the packed file - and the relocations it made; none of it is
# TIM.EXE's. And the unpacked header is unlzexe.py's own, so its size and
# `minalloc` describe that memory image, not the file LZEXE packed: only the
# entry and the stack, which LZEXE keeps, are the original's.
PROGRAM_END = 0x2D3C0 + 0x6550


def compare(built, original, owner):
    fb, bb, rb = mz(built)
    fo, bo, ro = mz(original)
    print("header (LZEXE keeps the entry and the stack; the rest is unlzexe.py's):")
    for k in ("cs", "ip", "ss", "sp"):
        mark = "" if fb[k] == fo[k] else "   <- differs"
        print("  %-12s ours %6x   original %6x%s" % (k, fb[k], fo[k], mark))
    n = min(len(bb), len(bo))
    diff = runs(bb[:n], bo[:n])
    ndiff = sum(e - s for s, e in diff)
    rest = bo[len(bb):PROGRAM_END]
    nonzero = sum(1 for x in rest if x)
    print("load module: %d of the %d bytes TLINK wrote differ, in %d runs; "
          "the original's next %d up to the stack's end, which TLINK does not "
          "write, hold %d non-zero bytes"
          % (ndiff, len(bb), len(diff), len(rest), nonzero))
    shown = {}
    for s, e in diff:
        shown.setdefault(owner(s), []).append((s, e))
    for who, rs in sorted(shown.items(), key=lambda x: x[1][0][0]):
        tot = sum(e - s for s, e in rs)
        print("  %-28s %6d bytes in %4d runs, first at %05x"
              % (who, tot, len(rs), rs[0][0]))
    ro_prog = {r for r in ro if r < PROGRAM_END}
    print("relocations: %d ours, %d the original's below the stack's end (%d "
          "more above it, LZEXE's), %d in both"
          % (len(rb), len(ro_prog), len(ro) - len(ro_prog), len(rb & ro_prog)))
    for r in sorted(ro_prog - rb)[:8]:
        print("  only the original's: %05x" % r)
    for r in sorted(rb - ro_prog)[:8]:
        print("  only ours: %05x" % r)
    return ndiff == 0 and nonzero == 0 and rb == ro_prog and \
        all(fb[k] == fo[k] for k in ("cs", "ip", "ss", "sp"))


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--reuse", action="store_true")
    ap.add_argument("-j", type=int, default=8)
    a = ap.parse_args(argv)
    os.makedirs(os.path.join(OUT, "obj"), exist_ok=True)
    files = sorted(game_files())
    objname = {f: os.path.join(OUT, "obj", "M%03d.OBJ" % k) for k, f in enumerate(files)}
    code = {f: first_address(f) for f in files}
    saved = os.path.join(OUT, "data.json")
    # `--reuse` still rebuilds a file whose source changed since its object
    # was built (the link rewrites every object, so its time says nothing);
    # a header's change needs the full build
    def digest(f):
        return hashlib.sha1(open(f, "rb").read()).hexdigest()
    old = {}
    if a.reuse and os.path.exists(saved):
        old = {os.path.join(REPO, k): v for k, v in json.load(open(saved)).items()}
    todo = files if not a.reuse else [
        f for f in files if not os.path.exists(objname[f])
        or old.get(f, {}).get("hash") != digest(f)]
    data = {f: old[f]["data"] for f in old if f not in todo}
    hashes = {f: old[f]["hash"] for f in old if f not in todo}
    if todo:
        bad = []
        with concurrent.futures.ThreadPoolExecutor(a.j) as ex:
            for path, ok, verdict, at in ex.map(lambda f: build(f, objname[f]), todo):
                data[path] = at
                hashes[path] = digest(path)
                m = re.search(r"(\d+) of (\d+) routines match", verdict)
                if not ok or not m or m.group(1) != m.group(2):
                    bad.append((path, verdict.strip()))
        for p, v in bad:
            print("NOT MATCHED:", os.path.relpath(p, REPO), v)
        print("%d modules built" % (len(todo) - len(bad)))
        json.dump({os.path.relpath(k, REPO): {"data": v, "hash": hashes.get(k)}
                   for k, v in data.items()}, open(saved, "w"))
    fr = judge.frames()
    order = object_order([(f, code[f], data.get(f)) for f in files], fr)
    objs = []
    for f, addr, _d in order:
        o = objname[f]
        if os.path.exists(o):
            one_code_segment(o, judge.frame_of(addr, fr) if addr is not None else 0)
            objs.append(o)
    order = [(addr, f, objname[f]) for f, addr, _d in order if addr is not None]
    log, exe = link(objs)
    tail = [l for l in log.splitlines() if l.strip() and "VIRTUAL_ENV" not in l]
    print("\n".join(tail[-15:]))
    if not os.path.exists(exe):
        raise SystemExit("TLINK wrote no TIM.EXE")
    starts = sorted((addr, os.path.relpath(f, REPO)) for addr, f, _o in order)

    def owner(off):
        who = "before the first module (start-up)"
        for addr, f in starts:
            if addr <= off:
                who = f
        if off >= 0x2d400:
            who = "past the code (DGROUP and after)"
        return who
    ok = compare(open(exe, "rb").read(), open(ORIGINAL, "rb").read(), owner)
    print("linked:", os.path.relpath(exe, REPO))
    print("IDENTICAL" if ok else "differs")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
