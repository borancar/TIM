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


def build(path, obj):
    """The judge on one file, keeping the object it built."""
    r = subprocess.run([sys.executable, os.path.join(HERE, "judge.py"), path],
                       cwd=REPO, capture_output=True, text=True,
                       env=dict(os.environ, JUDGE_KEEP_OBJ=obj))
    verdict = [l for l in r.stdout.splitlines() if "routines match" in l]
    return path, os.path.exists(obj), verdict[-1] if verdict else r.stdout[-300:]


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


def compare(built, original, owner):
    fb, bb, rb = mz(built)
    fo, bo, ro = mz(original)
    print("header:")
    for k in fo:
        mark = "" if fb[k] == fo[k] else "   <- differs"
        print("  %-12s ours %6x   original %6x%s" % (k, fb[k], fo[k], mark))
    n = min(len(bb), len(bo))
    diff = runs(bb[:n], bo[:n])
    ndiff = sum(e - s for s, e in diff) + abs(len(bb) - len(bo))
    print("load module: %d of %d bytes differ, in %d runs"
          % (ndiff, len(bo), len(diff)))
    shown = {}
    for s, e in diff:
        who = owner(s)
        shown.setdefault(who, []).append((s, e))
    for who, rs in sorted(shown.items(), key=lambda x: x[1][0][0]):
        tot = sum(e - s for s, e in rs)
        print("  %-28s %6d bytes in %4d runs, first at %05x"
              % (who, tot, len(rs), rs[0][0]))
    print("relocations: %d ours, %d original, %d in both"
          % (len(rb), len(ro), len(rb & ro)))
    return ndiff == 0 and rb == ro and fb == fo


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--reuse", action="store_true")
    ap.add_argument("-j", type=int, default=8)
    a = ap.parse_args(argv)
    os.makedirs(os.path.join(OUT, "obj"), exist_ok=True)
    files = sorted(((first_address(f), f) for f in game_files()),
                   key=lambda x: (x[0] is None, x[0] or 0))
    order = []
    for k, (addr, f) in enumerate(files):
        if addr is None:
            print("no address, left out:", f)
            continue
        order.append((addr, f, os.path.join(OUT, "obj", "M%03d.OBJ" % k)))
    if not a.reuse:
        bad = []
        with concurrent.futures.ThreadPoolExecutor(a.j) as ex:
            for path, ok, verdict in ex.map(lambda t: build(t[1], t[2]), order):
                m = re.search(r"(\d+) of (\d+) routines match", verdict)
                if not ok or not m or m.group(1) != m.group(2):
                    bad.append((path, verdict.strip()))
        for p, v in bad:
            print("NOT MATCHED:", os.path.relpath(p, REPO), v)
        print("%d modules built" % (len(order) - len(bad)))
    fr = judge.frames()
    objs = []
    for addr, _f, o in order:
        if os.path.exists(o):
            one_code_segment(o, judge.frame_of(addr, fr))
            objs.append(o)
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
