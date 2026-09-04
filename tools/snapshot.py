"""Capture and restore the whole machine, so a state reached by playing can be
replayed by a tool.

The problem this solves: the intros are the only part of the game the tools can
reach on their own. They run from the entry point, they need no input, and they
are what every measurement so far has been taken on - which is why coverage
stops at 72% of reachable code. The menus, the level editor and the game proper
are behind a person pressing keys, and reaching them by hand for every capture,
every verification sweep and every regression check is what makes them untested
rather than any difficulty in the code.

So: play once, capture, and from then on start there.

What is captured
----------------

- **The 2 MB of guest memory.** One flat Unicorn mapping, so it is one read.
- **The CPU registers**, segment registers and flags included.
- **The four VGA planes** and the register state around them: the map mask and
  active planes, the latches, the CRTC file and its index, the sequencer and
  graphics controller, the attribute palette and its flip-flop, the DAC and its
  write phase, the mode and its geometry, the start address and its unit.
- **The PIT and the retrace rate**: the divisor, the phase, when the next tick
  is due, the latch toggle, and `vsync_hz`, from which the emulator answers the
  vertical-retrace bit the game paces its page flips on. The game programs it to 236.7 Hz and divides that down to 18.2 for
  the BIOS, so a machine restored with a fresh PIT runs the intro's animation at
  the wrong speed.
- **The input state the game polls**: the key buffer, the scancode queue, the
  pending half of an extended key, the mouse position and buttons, and the
  per-button press and release counts and positions.
- **The DOS shim**: the memory arena, the open handles with their contents and
  positions, the PSP and environment segments, the DTA, the current directory,
  and any files the game has created.
- **The virtual clock**, as an instruction count.

What is deliberately not captured
---------------------------------

**Hooks are not state.** They are installed by `tim.TimMachine.__init__` and by
whatever tool built the machine, which is what lets a snapshot taken under one
tool be replayed under another.

**Counters and logs are not state.** `file_ops`, `dos_alloc_log`, `int_counts`,
`port_in`, `port_out`, `vidwrites` and the rest are diagnostics: they say what
the machine has done, not what it will do. Restoring them would make a replay
report the saved run's history as its own.

**The host's audio is not resumed.** `spk_chan` and `spk_playing` are pygame
handles; a restored machine starts silent and the game starts its own sounds.

**The clock is carried as elapsed *time*, and rebased on restore.** Not as a
raw counter, and this is the trap that nearly got through.

`tim.TimMachine._elapsed` is `vclock / vclock_ips` when `vclock_ips` is set -
which every headless tool sets, because it is what makes a capture
reproducible - and falls back to the host clock when it is not, which is what
the windowed run uses. So the two paths measure time in different units, and a
snapshot crosses between them: play in the window, restore in a tool.

Restoring the raw fields would put a windowed machine's `vclock_ips` of 0 into
a tool that had asked for determinism, silently. Worse, `pit0_next` - when the
next timer tick is due - is a reading of `_elapsed()`, about 30 seconds into a
played session; restored into a machine whose clock starts at 0 it says the
next tick is thirty seconds away, and the game sits waiting for a timer that
never comes and looks frozen.

So the manifest carries `elapsed`, the restoring machine keeps whatever clock
rate its *tool* asked for, and the clock is set so `_elapsed()` reads the same
number it did when the snapshot was taken. Everything derived from it - the
PIT, the retrace phase - stays continuous across the save.

**Nothing is captured that has not been named.** `save` compares the machine's
own attributes against the two lists below and refuses on anything in neither,
so a field added to the emulator upstream stops this file rather than being
silently dropped. A snapshot that quietly lost a register would be worse than
no snapshot: it would restore, run, and diverge.

This file is the port's own tooling; it is not a transcription.
"""
import base64
import json
import os
import struct
import sys
import zlib
from collections import deque

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_FS, UC_X86_REG_GS, UC_X86_REG_EFLAGS,
)

class SnapshotError(Exception):
    """Something a snapshot cannot carry.

    An ordinary exception and deliberately not `SystemExit`: `save` is called
    from inside the emulator's own event loop when someone presses shift+F2,
    and a refusal there has to be something the loop can print and carry on
    from. A failed save must not end a session that took a person minutes of
    play to reach.
    """


MAGIC = b"TIMSNAP1"

MEM_BASE = 0x00000000
MEM_SIZE = 0x00200000          # the one flat mapping, 2 MB

REGS = [
    ("ax", UC_X86_REG_AX), ("bx", UC_X86_REG_BX),
    ("cx", UC_X86_REG_CX), ("dx", UC_X86_REG_DX),
    ("si", UC_X86_REG_SI), ("di", UC_X86_REG_DI),
    ("bp", UC_X86_REG_BP), ("sp", UC_X86_REG_SP),
    ("ip", UC_X86_REG_IP), ("cs", UC_X86_REG_CS),
    ("ds", UC_X86_REG_DS), ("es", UC_X86_REG_ES),
    ("ss", UC_X86_REG_SS), ("fs", UC_X86_REG_FS),
    ("gs", UC_X86_REG_GS), ("eflags", UC_X86_REG_EFLAGS),
]

# Everything the machine's behaviour depends on. Plain values: ints, floats,
# strings, and containers of those.
SAVE = [
    # video
    "mode", "width", "height", "text_mode", "chain4", "active_page",
    "start_addr", "start_mult", "map_mask", "active_planes", "latches",
    "crtc", "crtc_index", "crtc_offset", "gc", "gc_index", "seq_index",
    "attr_pal", "attr_index", "attr_flipflop",
    "dac_index", "dac_latch", "dac_phase", "dac_write_mode", "palette",
    "cga_mode_ctrl", "cga_colour", "cursor", "video_modes",
    # timer and speaker
    "pit0_div", "pit0_next", "pit0_phase", "pit_initial", "pit_latch_toggle",
    "timer_ticks", "boot_int08", "boot_int09", "hooked_vectors",
    "spk_div", "spk_gate", "vsync_hz", "hsync_hz",
    "pit0_mode", "pit_latched", "pit_load", "pit_load_t",
    # How many reads of 0x3DA have come in a row with nothing else between
    # them. The emulator uses it to recognise a guest spinning on the retrace
    # and skip to the edge, which moves the virtual clock - so it is timing,
    # not diagnostics, and a restore that dropped it would take one spin
    # longer to be recognised.
    "da_streak",
    # input
    "key_buf", "scan_queue", "last_scancode", "pending_scan",
    "blocked_on_input", "mouse_pos", "mouse_btn", "mouse_rel", "mouse_sens",
    "mouse_x", "mouse_y",
    # The INT 33h user handler and the clamps, from tools/tim.py. The handler
    # is the *only* way a click reaches this game - it installs one with AX=0x0C
    # and never polls - so a restore without it is a machine the mouse cannot
    # talk to, which looks like a screen that has stopped responding rather
    # than like a lost snapshot field. The ranges are the AH=7/8 clamps and
    # decide what a click's coordinates become.
    "mouse_handler", "mouse_handler_mask", "mouse_x_range", "mouse_y_range",
    # Events accepted but not yet delivered to that handler. Normally empty at
    # a snapshot - it is drained on the next slice - but it is state, and one
    # left in it would otherwise be dropped.
    "_mouse_pending",
    "press_count", "press_pos", "release_count", "release_pos",
    # DOS
    "arena", "blocks", "mem_top", "start", "load_seg", "prog_paras",
    "prog_path", "psp_seg", "env_seg", "dta", "cwd", "cmdline",
    "dos_files", "next_handle", "overlay", "finds", "find_seq",
    "disk_status", "stdout", "finished",
    # the clock
    "vclock", "vclock_ips", "t0",
]

# Named, and deliberately left out. The docstring says why; this list is what
# stops `save` from complaining about them.
SKIP = [
    # hooks and live host objects, rebuilt by whoever makes the machine
    "uc", "_dma_hook", "_vidwrite_hook", "_trun", "xms",
    "sb", "spk_chan", "spk_playing", "sb_last_tick",
    # diagnostics: what the machine has done, not what it will do
    "log", "file_ops", "dos_alloc_log", "dos_counts", "int_counts",
    "int10_fn", "port_in", "port_out", "vidwrites", "vidrange",
    "mouse_calls", "guest_dispatch", "files_missing", "files_read", "frames",
    "files_written", "palette_writes", "sb_irqs", "_warned_range",
    "verbose", "max_insns",
    "planes", "handles",                  # both travel as blobs, below
]

# Restored as tuples: JSON has only arrays, and the emulator unpacks these.
TUPLES = {"active_planes", "boot_int08", "boot_int09", "dta", "mouse_pos"}
# Lists whose members are tuples.
TUPLE_LISTS = {"palette", "cursor"}
# Dicts whose keys are integers, which JSON turns into strings.
INT_KEYS = {"crtc", "pit_latch_toggle", "hooked_vectors", "dos_files"}
# Dict values that are tuples.
TUPLE_VALUES = {"hooked_vectors"}
# Deques, which must come back as deques or `popleft` is gone.
DEQUES = {"key_buf", "scan_queue"}


def _encodable(v):
    """A value the manifest can carry, or a raised error naming it."""
    if isinstance(v, (bytes, bytearray)):
        return {"__bytes__": base64.b64encode(bytes(v)).decode("ascii")}
    if isinstance(v, deque):
        return [_encodable(x) for x in v]
    if isinstance(v, (list, tuple)):
        return [_encodable(x) for x in v]
    if isinstance(v, dict):
        return {str(k): _encodable(x) for k, x in v.items()}
    if isinstance(v, (int, float, bool, str)) or v is None:
        return v
    raise TypeError("cannot snapshot a %s" % type(v).__name__)


def _decoded(v):
    if isinstance(v, dict) and set(v) == {"__bytes__"}:
        return bytearray(base64.b64decode(v["__bytes__"]))
    if isinstance(v, list):
        return [_decoded(x) for x in v]
    if isinstance(v, dict):
        return {k: _decoded(x) for k, x in v.items()}
    return v


def save(m, path):
    """Write the machine to `path`.

    Refuses rather than guesses. An attribute in neither `SAVE` nor `SKIP` is
    an emulator that has grown a field this file has not been taught, and
    carrying on would produce a snapshot that restores and then diverges.
    """
    unknown = sorted(set(vars(m)) - set(SAVE) - set(SKIP))
    if unknown:
        raise SnapshotError(
            "the machine has attributes this file does not know: %s\n"
            "Add each to SAVE (it affects behaviour) or to SKIP (it does not),\n"
            "and say which in the docstring." % ", ".join(unknown))

    if getattr(m, "xms", None) is not None:
        if m.xms.handles or m.xms.locks:
            raise SnapshotError("XMS blocks are allocated; carrying them is "
                                "not implemented")

    # An open handle is its *contents* and a position - the shim reads a file
    # once and serves it from memory - so it travels whole rather than as a
    # name to be reopened. That costs a few hundred kilobytes before
    # compression and removes every question about whether the file on disk is
    # still the file the saved run was reading.
    open_files = [{"handle": h,
                   "path": f.path,
                   "pos": f.pos,
                   "writable": f.writable,
                   "written": f.written}
                  for h, f in sorted(m.handles.items())]

    manifest = {
        "magic": "TIMSNAP1",
        # What `_elapsed()` read when this was taken. See the docstring: the
        # units differ between a windowed run and a headless one, so the
        # reading travels and the counter behind it is rebuilt on restore.
        "elapsed": m._elapsed(),
        "regs": {name: m.uc.reg_read(r) for name, r in REGS},
        "state": {k: _encodable(getattr(m, k)) for k in SAVE if hasattr(m, k)},
        "open_files": open_files,
    }

    blobs = [bytes(m.uc.mem_read(MEM_BASE, MEM_SIZE))]
    blobs += [bytes(p) for p in m.planes]
    blobs += [bytes(m.handles[e["handle"]].data) for e in open_files]

    manifest["blobs"] = [len(b) for b in blobs]
    packed = [zlib.compress(b, 6) for b in blobs]
    manifest["packed"] = [len(b) for b in packed]

    body = json.dumps(manifest).encode("utf-8")
    with open(path, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<I", len(body)))
        f.write(body)
        for b in packed:
            f.write(b)
    return path


def restore(m, path):
    """Put `path` back into an already-built machine.

    The machine keeps the clock rate it was built with - a tool that asked for
    a virtual clock keeps it, whatever the saved run was using - and the clock
    itself is set so the guest's sense of time carries on from the save.
    """
    import time

    want_ips = getattr(m, "vclock_ips", 0)

    with open(path, "rb") as f:
        if f.read(8) != MAGIC:
            raise SnapshotError("%s is not a snapshot" % path)
        (mlen,) = struct.unpack("<I", f.read(4))
        manifest = json.loads(f.read(mlen).decode("utf-8"))
        blobs = [zlib.decompress(f.read(n)) for n in manifest["packed"]]

    m.uc.mem_write(MEM_BASE, blobs[0])
    for i in range(4):
        m.planes[i] = bytearray(blobs[1 + i])

    from dos_emulator.emulator import Handle

    m.handles = {}
    for i, e in enumerate(manifest.get("open_files", [])):
        h = Handle(e["path"], blobs[5 + i], e["writable"])
        h.pos = e["pos"]
        h.written = e["written"]
        m.handles[e["handle"]] = h

    for k, v in manifest["state"].items():
        v = _decoded(v)
        if k in INT_KEYS:
            v = {int(kk): vv for kk, vv in v.items()}
        if k in TUPLE_VALUES:
            v = {kk: tuple(vv) for kk, vv in v.items()}
        if k in TUPLES:
            v = tuple(v)
        if k in TUPLE_LISTS:
            v = [tuple(x) for x in v]
        if k in DEQUES:
            v = deque(v)
        setattr(m, k, v)

    for name, r in REGS:
        m.uc.reg_write(r, manifest["regs"][name])

    elapsed = manifest.get("elapsed", 0.0)
    m.vclock_ips = want_ips
    if want_ips:
        m.vclock = int(round(elapsed * want_ips))
    else:
        m.t0 = time.perf_counter() - elapsed

    return m


def load(path, **kw):
    """Build a machine and restore `path` into it."""
    import drive

    m = drive.machine(**kw)
    return restore(m, path)


# --------------------------------------------------------------- self-test
#
# A snapshot that restores and *looks* right is worth nothing: the failure it
# has to be checked against is a machine that carries on and slowly diverges,
# which no single inspection catches. So the test is the project's own method -
# run the thing twice and compare what each did.
#
# Run to flip A saving a snapshot, then keep going to flip B and keep every
# frame. Restore the snapshot into a fresh machine, run it to flip B, and keep
# every frame. Every frame from A to B must be identical, and so must the
# instruction count at which each flip happened. Frames alone would miss a
# machine that draws the same picture a little later.


def _run_to_flips(m, first, last, insns=4_000_000_000):
    """Frames and clock readings for flips `first`..`last`, inclusive."""
    import drive
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    got = {}
    st = {"n": 0}

    def on_out(uc, port, size, value, ud):
        if port != 0x3D4 or size != 2 or (value & 0xFF) != 0x0C:
            return
        n = st["n"]
        st["n"] = n + 1
        if first <= n <= last:
            got[n] = (m.framebuffer(), m.vclock)

    h = m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, insns, on_slice=lambda mm, d: st["n"] > last)
    m.uc.hook_del(h)
    return got


def selftest(at=60, until=90, path=None, keep=False):
    import tempfile
    import drive

    path = path or os.path.join(tempfile.gettempdir(), "tim-selftest.snap")

    # One uninterrupted run: save at `at`, and keep the frames after it.
    m = drive.machine()
    st = {"n": 0, "saved": False}

    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    def on_out(uc, port, size, value, ud):
        if port != 0x3D4 or size != 2 or (value & 0xFF) != 0x0C:
            return
        st["n"] += 1
        if st["n"] == at and not st["saved"]:
            st["saved"] = True
            st["stop"] = True

    h = m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, 4_000_000_000, on_slice=lambda mm, d: st.get("stop"))
    m.uc.hook_del(h)
    print("reached flip %d at %d instructions" % (st["n"], m.vclock))

    save(m, path)
    print("snapshot %s: %d bytes" % (path, os.path.getsize(path)))

    straight = _run_to_flips(m, at, until)
    print("straight through: %d frames from flip %d" % (len(straight), at))

    m2 = load(path)
    restored = _run_to_flips(m2, at, until)
    print("after restore:    %d frames from flip %d" % (len(restored), at))

    bad = 0
    for n in sorted(set(straight) | set(restored)):
        a, b = straight.get(n), restored.get(n)
        if a is None or b is None:
            print("  flip %d: only one run reached it" % n)
            bad += 1
            continue
        if a[0] != b[0]:
            d = sum(1 for x, y in zip(a[0], b[0]) if x != y)
            print("  flip %d: %d of %d indices differ" % (n, d, len(a[0])))
            bad += 1
        elif a[1] != b[1]:
            print("  flip %d: same frame, clock %d vs %d (%+d)"
                  % (n, a[1], b[1], b[1] - a[1]))
            bad += 1

    if not keep:
        os.unlink(path)
    if bad:
        print("FAIL: %d of %d flips differ" % (bad, len(straight)))
        return 1
    print("OK: %d flips identical, frame and clock, across the restore"
          % len(straight))
    return 0


def save_at_flip(flip, path, clicks=()):
    """Run from the entry point and save the machine at `flip`.

    `clicks` is the same `(flip, x, y)` list `capture.capture_flips` takes and
    `TIM_CLICK` parses, pressed and released at the same flips. Without it this
    reaches only what the game gets to on its own, which is the intros and the
    first screen - and **everything behind a menu is behind a click**, so every
    routine in the file picker and the save path was unreachable for
    `verify.py` until this existed.

    The clicks are placed at flips rather than at times for the reason they are
    everywhere else here: a flip is the one clock both sides of this project
    agree on, so the same list drives the port and the original to the same
    place.
    """
    import drive
    from unicorn import UC_HOOK_INSN
    import unicorn.x86_const as xc

    m = drive.machine()
    st = {"n": 0}

    def on_out(uc, port, size, value, ud):
        if port != 0x3D4 or size != 2 or (value & 0xFF) != 0x0C:
            return
        n = st["n"]
        st["n"] = n + 1
        for at, cx, cy in clicks:
            if n == at:
                m.mouse_input(cx, cy, 1)
            elif n == at + 2:
                m.mouse_input(cx, cy, 0)
        if st["n"] >= flip:
            st["stop"] = True

    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    drive.drive(m, 4_000_000_000, on_slice=lambda mm, d: st.get("stop"))
    if st["n"] < flip:
        raise SystemExit("only reached flip %d" % st["n"])

    save(m, path)
    print("flip %d, %d instructions -> %s (%d bytes)"
          % (st["n"], m.vclock, path, os.path.getsize(path)))
    return 0


def next_snapshot_path(prefix, directory=None):
    """The next free `<prefix>NNN.snap`, the way the C side names its own.

    The port writes `tim000.snap` and `devtim000.snap`, the hybrid runner
    writes `native000.snap`, and this writes `emulator000.snap` - each program
    numbering its own, so a name says where a capture came from without opening
    it. The number is found by looking rather than remembered, so it survives a
    restart and never overwrites an earlier run.

    Not the same *format* as either of those: this is the Python machine and
    they are not. The convention is shared; the file is not interchangeable.
    """
    directory = directory or os.environ.get("TIM_SNAPDIR") or "out"
    os.makedirs(directory, exist_ok=True)
    for i in range(1000):
        path = os.path.join(directory, "%s%03d.snap" % (prefix, i))
        if not os.path.exists(path):
            return path
    return os.path.join(directory, "%s999.snap" % prefix)


def main():
    import argparse

    ap = argparse.ArgumentParser(
        description=__doc__.split("\n")[0],
        epilog="environment:\n"
               "  TIM_SNAPDIR=DIR  where a numbered snapshot goes when --out\n"
               "                   does not name one (default out). The same\n"
               "                   variable devtim and the hybrid read.\n",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--click", action="append", metavar="FLIP:X:Y",
                    help="press the left button at this flip and let it go "
                         "two flips later; may be repeated. The same form "
                         "TIM_CLICK takes, so one list drives both sides")
    ap.add_argument("--save-at-flip", type=int, default=None, metavar="N",
                    help="run from the entry point and save the machine at "
                         "this page flip. Reaches anything the game gets to on "
                         "its own; for a state behind a menu, drive it there "
                         "with the emulator's own --keys and its reproducible "
                         "@seg:off form (see tools/run.py --help) and save "
                         "from that run instead of from this one")
    ap.add_argument("--selftest", action="store_true",
                    help="prove a restore is indistinguishable from not "
                         "having stopped")
    ap.add_argument("--at", type=int, default=60, help="flip to snapshot at")
    ap.add_argument("--until", type=int, default=90, help="flip to compare to")
    ap.add_argument("--out", default=None, help="where to write the snapshot")
    ap.add_argument("--keep", action="store_true", help="keep the test file")
    args = ap.parse_args()

    if args.selftest:
        return selftest(args.at, args.until, args.out, args.keep)

    if args.save_at_flip is not None:
        if not args.out:
            args.out = next_snapshot_path("emulator")
            print("writing %s" % args.out)
        clicks = []
        for spec in (args.click or []):
            at, cx, cy = (int(v, 0) for v in spec.split(":"))
            clicks.append((at, cx, cy))
        return save_at_flip(args.save_at_flip, args.out, clicks)

    ap.error("nothing to do; try --save-at-flip or --selftest")


if __name__ == "__main__":
    sys.exit(main())
