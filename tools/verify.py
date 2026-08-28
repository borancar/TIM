"""Prove a transcribed routine against the original, one routine at a time.

The emulator is stopped at the routine's entry, the **original body** is allowed
to run to its return, and everything it does to the hardware is recorded. The
transcribed C is then called with the same arguments and its own hardware
accesses are recorded the same way. The two sequences must be identical, event
for event.

Comparing the hardware trace rather than the machine's registers is what makes
this tractable: it needs no mapping of the original's whole state into the
port's, and for this game it is the thing that actually matters, since the
screens are produced entirely by writes to the VGA.

It also needs no determinism to mean anything - it compares the C and the
original on *the same call inside one run*, so the host clock and the game's
own timing cannot make it flaky.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import ctypes
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive

from unicorn import (UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_READ,
                     UC_HOOK_MEM_WRITE)
from unicorn.x86_const import (UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SS,
                               UC_X86_REG_SP, UC_X86_REG_AX, UC_X86_REG_DX)
import unicorn.x86_const as xc

LIB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "reconstruct", "libtim.so")


class Event(ctypes.Structure):
    _fields_ = [("port", ctypes.c_uint16), ("offset", ctypes.c_uint16),
                ("value", ctypes.c_uint8), ("is_read", ctypes.c_uint8)]


def load_lib():
    if not os.path.exists(LIB):
        raise SystemExit("no %s - run `make libtim.so` in reconstruct/" % LIB)
    lib = ctypes.CDLL(LIB)
    lib.io_trace_count.restype = ctypes.c_int32
    lib.io_trace_events.restype = ctypes.POINTER(Event)
    return lib


def port_trace(lib, call):
    lib.io_reset()
    lib.io_trace_begin()
    call(lib)
    n = lib.io_trace_count()
    ev = lib.io_trace_events()
    return [(ev[i].port, ev[i].offset, ev[i].value, ev[i].is_read)
            for i in range(n)]


# Each entry says how to call the C and how to read the original's arguments
# off its stack at entry. A far call has ret IP at SP, ret CS at SP+2, and the
# first argument at SP+4.
DGROUP = 0x2D3C0

ROUTINES = {
    "vm_set_display_lines": dict(
        addr=0x08F77,
        args=[("lines", 4)],
        call=lambda lib, a: lib.vm_set_display_lines(ctypes.c_uint16(a[0])),
    ),
    # No hardware effect at all: its whole result is the value it returns, so
    # the trace comparison would find "0 writes on both sides" and call it
    # agreement. That is the shallow-agreement trap, so this one is checked on
    # its return value, with the DGROUP word it reads seeded from the
    # original's own memory at the moment of the call.
    "frame_pending": dict(
        addr=0x0B4E2,
        args=[],
        state=[("frame_flag", 0x5754, 2)],
        returns=True,
        call=lambda lib, a: lib.frame_pending(),
    ),
}


def original_trace(m, addr, nargs, want_state=None, occurrence=0,
                   budget=40_000_000):
    """Run until the routine is entered, then record what the original does."""
    base = m.load_seg * 16
    entry = base + addr
    st = {"in": False, "hits": 0, "args": None, "events": [], "done": False,
          "ret": None, "sp": None, "ax": None, "state": {},
          "want_state": want_state or []}

    def on_code(uc, address, size, ud):
        if st["done"]:
            return
        if address == entry and not st["in"]:
            if st["hits"] < occurrence:
                st["hits"] += 1
                return
            ss = uc.reg_read(UC_X86_REG_SS)
            sp = uc.reg_read(UC_X86_REG_SP)
            stk = uc.mem_read(ss * 16 + sp, 4 + 2 * max(1, nargs))
            st["ret"] = (stk[0] | (stk[1] << 8), stk[2] | (stk[3] << 8))
            st["sp"] = sp
            st["args"] = [stk[4 + 2 * i] | (stk[5 + 2 * i] << 8)
                          for i in range(nargs)]
            dg = base + DGROUP
            st["state"] = {off: int.from_bytes(uc.mem_read(dg + off, size), "little")
                           for (_, off, size) in st["want_state"]}
            st["in"] = True
            return
        if st["in"]:
            # Returned when control is back at the pushed CS:IP with the stack
            # unwound past the far return address.
            cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
            if (ip, cs) == st["ret"] and uc.reg_read(UC_X86_REG_SP) >= st["sp"] + 4:
                st["in"] = False
                st["done"] = True
                st["ax"] = uc.reg_read(UC_X86_REG_AX)

    def on_out(uc, port, size, value, ud):
        if st["in"]:
            st["events"].append((port, 0, value & 0xFF, 0))

    def on_in(uc, port, size, ud):
        return None

    def on_mem(uc, typ, address, size, value, ud):
        if st["in"] and 0xA0000 <= address < 0xB0000:
            st["events"].append((0xA000, address - 0xA0000,
                                 value & 0xFF if typ == 17 else 0,
                                 0 if typ == 17 else 1))

    m.uc.hook_add(UC_HOOK_CODE, on_code)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)
    # Range-limited to the VGA aperture. Registered across all of memory these
    # fire on every read and write the guest makes, which is both ruinously
    # slow and - as this project found - enough to derail the guest entirely:
    # it opened a file with a garbage name and then executed an invalid
    # instruction.
    m.uc.hook_add(UC_HOOK_MEM_WRITE, on_mem, None, 0xA0000, 0xB0000)
    m.uc.hook_add(UC_HOOK_MEM_READ, on_mem, None, 0xA0000, 0xB0000)

    def stop(mm, done):
        return st["done"]

    st["why"] = drive.drive(m, budget, on_slice=stop)
    return st


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("routine", nargs="?", default=None)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--occurrence", type=int, default=0,
                    help="check the Nth call rather than the first. A routine "
                         "checked at one value of its inputs says nothing "
                         "about the others")
    args = ap.parse_args()

    if args.list or not args.routine:
        for k, v in ROUTINES.items():
            print("  %-28s 0x%05x" % (k, v["addr"]))
        return 0

    spec = ROUTINES[args.routine]
    lib = load_lib()
    m = drive.machine()
    st = original_trace(m, spec["addr"], len(spec["args"]),
                        want_state=spec.get("state"),
                        occurrence=args.occurrence)

    # "Not entered" and "entered but never seen to return" are different
    # findings and must not print the same message - a check that cannot tell
    # them apart sends you looking in the wrong place.
    if st["args"] is None:
        print("  run ended: %s" % (st.get("why") or "budget exhausted"))
        print("%s: NOT ENTERED - the routine was never called within the "
              "instruction budget. That is not a pass; it is unchecked."
              % args.routine)
        return 2
    if not st["done"]:
        print("%s: ENTERED at %s but the return was never detected."
              % (args.routine, "0x%05x" % spec["addr"]))
        print("  entry SP=%#06x expecting return to %04x:%04x"
              % (st["sp"], st["ret"][1], st["ret"][0]))
        print("  %d hardware events recorded before the budget ran out"
              % len(st["events"]))
        return 2

    print("%s at 0x%05x" % (args.routine, spec["addr"]))
    print("  arguments: %s"
          % ", ".join("%s=%#06x" % (n, v)
                      for (n, _), v in zip(spec["args"], st["args"])))

    # The original's IN results have to be replayed into the port, or the two
    # cannot agree: the port's register file is not the emulator's.
    # Seed the port with the DGROUP values the original was actually looking
    # at when it was called - otherwise the two are not being asked the same
    # question.
    for name, off, size in spec.get("state", []):
        v = st["state"][off]
        sym = ctypes.c_int16.in_dll(lib, name) if size == 2 \
            else ctypes.c_int8.in_dll(lib, name)
        sym.value = v
        print("  state    : %s = %#06x (DGROUP %#06x)" % (name, v, off))

    lib.frame_pending.restype = ctypes.c_int16
    got_all = port_trace(lib, lambda l: spec["call"](l, st["args"]))
    want_all = st["events"]

    # Compare **writes**. A port or memory *read* has no external effect of its
    # own: it can only change behaviour through a write that follows it, so the
    # writes are the complete observable and a read that differed would show up
    # here anyway. Reads are counted and reported, never silently dropped.
    want = [e for e in want_all if not e[3]]
    got = [e for e in got_all if not e[3]]

    print("  original : %d writes (%d reads, not compared - see the source)"
          % (len(want), len(want_all) - len(want)))
    print("  port     : %d writes (%d reads)"
          % (len(got), len(got_all) - len(got)))

    n = max(len(want), len(got))
    bad = 0
    for i in range(n):
        w = want[i] if i < len(want) else None
        g = got[i] if i < len(got) else None
        if w != g:
            bad += 1
            if bad <= 12:
                print("    %3d  original %s   port %s" % (i, fmt(w), fmt(g)))
    if spec.get("returns"):
        lib.io_reset()
        for name, off, size in spec.get("state", []):
            ctypes.c_int16.in_dll(lib, name).value = st["state"][off]
        rv = spec["call"](lib, st["args"])
        want_ax = st["ax"] & 0xFFFF
        got_ax = rv & 0xFFFF
        print("  return   : original AX=%#06x  port=%#06x  %s"
              % (want_ax, got_ax, "ok" if want_ax == got_ax else "DIFFERS"))
        if want_ax != got_ax:
            return 1

    if bad == 0:
        print("  AGREED: %d events identical" % len(want))
        if not want and not spec.get("returns"):
            print("  NOTE: nothing was written and there is no return value to "
                  "compare - this call did no work, so an agreement here is "
                  "not evidence")
        return 0
    print("  DIFFERS in %d of %d events" % (bad, n))
    return 1


def fmt(e):
    if e is None:
        return "-"
    port, off, val, rd = e
    if port == 0xA000:
        return "A000:%04x %s %02x" % (off, "read " if rd else "write", val)
    return "%s %#05x = %02x" % ("in " if rd else "out", port, val)


if __name__ == "__main__":
    sys.exit(main())
