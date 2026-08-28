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
                               UC_X86_REG_SP, UC_X86_REG_AX, UC_X86_REG_BX,
                               UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI,
                               UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_DS,
                               UC_X86_REG_ES, UC_X86_REG_EFLAGS)

# Some driver routines take their arguments in registers rather than on the
# stack - they are reached through the vector table but they are not C
# functions. The transcription takes the same values as parameters.
REGS = {"ax": UC_X86_REG_AX, "bx": UC_X86_REG_BX, "cx": UC_X86_REG_CX,
        "dx": UC_X86_REG_DX, "si": UC_X86_REG_SI, "di": UC_X86_REG_DI,
        "bp": UC_X86_REG_BP, "ds": UC_X86_REG_DS, "es": UC_X86_REG_ES,
        "flags": UC_X86_REG_EFLAGS}
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


def port_trace(lib, call, setup=None):
    lib.io_reset()
    # Anything the port needs seeded must be seeded *after* io_reset, or it
    # wipes it. Doing it before cost a round: the planes were loaded from the
    # original and then cleared a moment later, and the copy read zeros.
    if setup is not None:
        setup(lib)
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
        check_occurrences=[0, 1],
        args=[("lines", 4)],
        call=lambda lib, a: lib.vm_set_display_lines(ctypes.c_uint16(a[0])),
    ),
    # No hardware effect at all: its whole result is the value it returns, so
    # the trace comparison would find "0 writes on both sides" and call it
    # agreement. That is the shallow-agreement trap, so this one is checked on
    # its return value, with the DGROUP word it reads seeded from the
    # original's own memory at the moment of the call.
    # An overlay routine: the loader chooses where VM.OVL goes, so there is no
    # fixed image address. The segment is resolved at run time by seeing who
    # writes to A000 - every pixel this game draws comes from this driver.
    "vm_show_page": dict(
        overlay=0x150F,
        check_occurrences=[0, 3, 9],
        args=[("wait_retrace", 4)],
        driver_state=[("vga_page_back", 0x12, 2),
                      ("vga_page_front", 0x14, 2),
                      ("vga_screen_height", 0x6EC, 2)],
        call=lambda lib, a: lib.vm_show_page(ctypes.c_uint16(a[0])),
    ),
    "vm_copy_rect": dict(
        overlay=0x1561,
        planes=True,
        args=[("x", 4), ("y", 6), ("width", 8), ("height", 10)],
        driver_state=[("vga_page_src", 0x16, 2),
                      ("vga_page_dst", 0x18, 2),
                      ("vga_row_offset", 0x6F2, 1024)],
        check_occurrences=[0, 2, 5],
        call=lambda lib, a: lib.vm_copy_rect(*[ctypes.c_uint16(v) for v in a]),
    ),
    # Register arguments: AL colour, BX x, CX count, ES:DI the row.
    "vm_span": dict(
        overlay=0x034F,
        args=[],
        regs=["ax", "bx", "cx", "es", "di"],
        planes=True,
        # 0 byte-aligned multi-byte; 4 and 17 unaligned multi-byte; 40 ends
        # exactly on a byte boundary; 9 and 73 are the single-byte path, which
        # none of the others reach. Found by scanning the arguments of all
        # 67,970 calls rather than by hoping.
        check_occurrences=[0, 4, 9, 17, 40, 73],
        call=lambda lib, a: lib.vm_span(ctypes.c_uint16(a[0]),
                                        ctypes.c_uint16(a[1]),
                                        ctypes.c_int16(a[2] if a[2] < 0x8000
                                                       else a[2] - 0x10000),
                                        ctypes.c_uint16(a[3]),
                                        ctypes.c_uint16(a[4])),
    ),
    "vm_blit_run": dict(
        overlay=0x0938,
        args=[],
        regs=["bx", "cx", "es", "di", "flags"],
        src_from=("ds", "si", "cx"),
        planes=True,
        # 0 and 2 are one- and two-pixel runs; 19 is a 26-pixel run starting
        # at bit 6, so the single-bit mask wraps three times; 3359 and 3360
        # are the *backward* direction, the horizontal flip, which none of the
        # forward ones reach. Found by scanning the arguments and the carry
        # flag of every call.
        check_occurrences=[0, 2, 19, 3359, 3360],
        budget=140_000_000,
        call=lambda lib, a: lib.vm_blit_run(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]), a[5],
            ctypes.c_uint16(a[2]), ctypes.c_uint16(a[3]),
            ctypes.c_int32(a[4] & 1)),
    ),
    "vm_fill_spans": dict(
        overlay=0x0be6,
        args=[],
        regs=["es", "si"],
        # The span list is a stream whose length depends on its own header, so
        # a generous fixed read is handed over; only what the routine actually
        # reads can affect the comparison.
        src_from=("es", "si", 8192),
        driver_state=[("vga_page_dst", 0x18, 2),
                      ("vga_fill_colour", 0x0D, 1),
                      ("vga_row_offset", 0x6F2, 1024)],
        planes=True,
        # 300 needs a bigger budget than the default: the routine is called
        # 1,078 times in all, but not that often in the first 40M
        # instructions. The sweep reported it NOT ENTERED rather than passing
        # it, which is the distinction that makes an unchecked routine
        # visible.
        check_occurrences=[0, 1, 40, 300],
        budget=150_000_000,
        call=lambda lib, a: lib.vm_fill_spans(a[2]),
    ),
    "vm_set_palette": dict(
        overlay=0x0EC1,
        args=[("rgb", 4), ("first", 6), ("count", 8)],
        src_stack=("ds", 0, 768),
        check_occurrences=[0, 1, 3],
        budget=120_000_000,
        call=lambda lib, a: lib.vm_set_palette(a[3], ctypes.c_uint16(a[1]),
                                               ctypes.c_uint16(a[2])),
    ),
    # The game's own wrapper around the page flip, so it needs both the
    # DGROUP flags it branches on and the driver state the flip uses.
    "present_frame": dict(
        addr=0x081CC,
        args=[("wait_retrace", 4)],
        state=[("present_hook_a", 0x52FA, 2),
               ("present_hook_b", 0x52F2, 2)],
        driver_state=[("vga_page_back", 0x12, 2),
                      ("vga_page_front", 0x14, 2),
                      ("vga_screen_height", 0x6EC, 2)],
        check_occurrences=[0, 5, 20],
        call=lambda lib, a: lib.present_frame(ctypes.c_uint16(a[0])),
    ),
    # The game's clipped rectangle fill. It needs the drawing state it reads
    # from DGROUP, and the driver state its span fill uses.
    "fill_rect": dict(
        addr=0x20079,
        args=[("x", 4), ("y", 6), ("w", 8), ("h", 10)],
        state=[("fill_enabled", 0x389C, 1),
               ("clip_enabled", 0x3893, 1),
               ("clip_left", 0x3894, 2),
               ("clip_right", 0x3896, 2),
               ("clip_top", 0x3898, 2),
               ("clip_bottom", 0x389A, 2),
               ("border_colour_a", 0x389D, 1),
               ("border_colour_b", 0x389E, 1)],
        driver_state=[("vga_page_dst", 0x18, 2),
                      ("vga_fill_colour", 0x0D, 1),
                      ("vga_row_offset", 0x6F2, 1024)],
        planes=True,
        check_occurrences=[0, 3, 60, 900],
        budget=150_000_000,
        call=lambda lib, a: lib.fill_rect(*[ctypes.c_int16(
            v if v < 0x8000 else v - 0x10000) for v in a]),
    ),
    "step_word_4e87": dict(
        addr=0x0144E,
        args=[],
        state=[("word_4e87", 0x4E87, 2)],
        check_occurrences=[0, 5, 60],
        call=lambda lib, a: lib.step_word_4e87(),
    ),
    "set_clip_full_screen": dict(
        addr=0x0834B,
        args=[],
        state=[("clip_left", 0x3894, 2), ("clip_right", 0x3896, 2),
               ("clip_top", 0x3898, 2), ("clip_bottom", 0x389A, 2)],
        # One occurrence, deliberately. The routine takes no arguments and has
        # no branches - it stores four constants - so a second call cannot
        # reach anything the first did not. It is also called only four times
        # in 200M instructions, so a later occurrence costs a long run for no
        # extra coverage.
        check_occurrences=[0],
        call=lambda lib, a: lib.set_clip_full_screen(),
    ),
    "frame_pending": dict(
        addr=0x0B4E2,
        check_occurrences=[0, 1],
        args=[],
        state=[("frame_flag", 0x5754, 2)],
        returns=True,
        call=lambda lib, a: lib.frame_pending(),
    ),
}


def original_trace(m, addr, nargs, want_state=None, occurrence=0,
                   budget=40_000_000, overlay_off=None, driver_state=None,
                   want_planes=False, reg_args=None, src_from=None,
                   src_stack=None):
    """Run until the routine is entered, then record what the original does."""
    base = m.load_seg * 16
    entry = None if overlay_off is not None else base + addr
    st = {"in": False, "hits": 0, "args": None, "events": [], "done": False,
          "ret": None, "sp": None, "ax": None, "state": {},
          "want_state": want_state or [], "drv": {}, "drv_seg": None,
          "want_drv": driver_state or [], "planes_in": None,
          "planes_out": None, "gc_in": None, "mask_in": 0x0F,
          "src": None, "state_out": None}

    def on_code(uc, address, size, ud):
        if st["done"]:
            return
        e = entry
        if e is None:
            if st["drv_seg"] is None:
                return
            e = st["drv_seg"] * 16 + overlay_off
        if address == e and not st["in"]:
            if st["hits"] < occurrence:
                st["hits"] += 1
                return
            ss = uc.reg_read(UC_X86_REG_SS)
            sp = uc.reg_read(UC_X86_REG_SP)
            stk = uc.mem_read(ss * 16 + sp, 4 + 2 * max(4, nargs))
            st["ret"] = (stk[0] | (stk[1] << 8), stk[2] | (stk[3] << 8))
            st["sp"] = sp
            if reg_args:
                st["args"] = [uc.reg_read(REGS[r]) for r in reg_args]
            else:
                st["args"] = [stk[4 + 2 * i] | (stk[5 + 2 * i] << 8)
                              for i in range(nargs)]
            # A source buffer, whether its pointer arrived in registers or on
            # the stack. This sits outside the branch above: putting it inside
            # meant a stack-argument routine silently got no buffer at all.
            if src_stack:
                sseg, aidx, slen = src_stack
                off = stk[4 + 2 * aidx] | (stk[5 + 2 * aidx] << 8)
                st["src"] = bytes(uc.mem_read(
                    uc.reg_read(REGS[sseg]) * 16 + off, slen))
            if src_from:
                # The blitter's source is ordinary memory, not video memory,
                # so the port is handed the same bytes rather than a segment
                # it has no way to reach.
                sseg, soff, slen = src_from
                n = (slen if isinstance(slen, int)
                     else uc.reg_read(REGS[slen]) or 0x10000)
                st["src"] = bytes(uc.mem_read(
                    uc.reg_read(REGS[sseg]) * 16 + uc.reg_read(REGS[soff]),
                    min(n, 0x10000)))
            dg = base + DGROUP
            st["state"] = {off: int.from_bytes(uc.mem_read(dg + off, size), "little")
                           for (_, off, size) in st["want_state"]}
            if want_planes:
                st["planes_in"] = [bytes(p) for p in m.planes]
                # The Graphics Controller and the map mask decide what a read
                # returns and which planes a write reaches. Without them the
                # port reads plane 0 while the original reads whichever its
                # read-map selects: the copy still comes out identical,
                # because latch mode ignores the value, but the recorded
                # bytes disagree and the check cannot tell that from a real
                # difference.
                st["gc_in"] = bytes(m.gc[:9])
                st["mask_in"] = m.map_mask
            if st["want_drv"]:
                # The driver loads its own data segment from cs:[0x13a]; at the
                # routine's first instruction DS is still the caller's.
                dseg = int.from_bytes(
                    uc.mem_read(st["drv_seg"] * 16 + 0x13A, 2), "little")
                st["drv"] = {}
                for (_, off, size) in st["want_drv"]:
                    raw = bytes(uc.mem_read(dseg * 16 + off, size))
                    st["drv"][off] = (raw if size > 2
                                      else int.from_bytes(raw, "little"))
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
                if want_planes:
                    st["planes_out"] = [bytes(p) for p in m.planes]
                if st["want_state"]:
                    # The state *after* the original ran. For a routine whose
                    # only effect is on DGROUP - a counter, a clip box - this
                    # is the whole of what there is to compare, and without it
                    # such a routine can only be reported as "agreed, 0
                    # events", which is no evidence at all.
                    dg2 = base + DGROUP
                    st["state_out"] = {
                        off: int.from_bytes(uc.mem_read(dg2 + off, size),
                                            "little")
                        for (_, off, size) in st["want_state"]}

    def on_out(uc, port, size, value, ud):
        if not st["in"]:
            return
        if size == 2:
            # A 16-bit OUT to an index port is one instruction and two
            # register writes. The port does them as two 8-bit writes, so
            # record it the same way or the two can never line up.
            st["events"].append((port, 0, value & 0xFF, 0))
            st["events"].append((port + 1, 0, (value >> 8) & 0xFF, 0))
        else:
            st["events"].append((port, 0, value & 0xFF, 0))

    def on_in(uc, port, size, ud):
        return None

    def on_mem(uc, typ, address, size, value, ud):
        if st["drv_seg"] is None and typ == 17:
            cs = uc.reg_read(UC_X86_REG_CS)
            if not (base <= cs * 16 < base + DGROUP):
                st["drv_seg"] = cs
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

    # An interrupt that fires *while the routine is running* writes to the
    # hardware too - the timer handler's end-of-interrupt to port 0x20 turned
    # up in the middle of a retrace wait and read as three spurious events.
    # Those writes are not the routine's, so the machine is not serviced while
    # it is inside one. This isolates the routine, which is the point.
    real_timer, real_kbd = m.service_timer, m.service_keyboard

    def gated_timer():
        return False if st["in"] else real_timer()

    def gated_kbd():
        return False if st["in"] else real_kbd()

    m.service_timer, m.service_keyboard = gated_timer, gated_kbd

    def stop(mm, done):
        return st["done"]

    st["why"] = drive.drive(m, budget, on_slice=stop)
    return st


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("routine", nargs="?", default=None)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="verify every routine and write the table into "
                         "STATUS.md, so the numbers there are measured rather "
                         "than retyped")
    ap.add_argument("--occurrence", type=int, default=0,
                    help="check the Nth call rather than the first. A routine "
                         "checked at one value of its inputs says nothing "
                         "about the others")
    args = ap.parse_args()

    if args.all:
        return sweep()

    if args.list or not args.routine:
        for k, v in ROUTINES.items():
            print("  %-28s 0x%05x" % (k, v["addr"]))
        return 0

    spec = ROUTINES[args.routine]
    lib = load_lib()
    m = drive.machine()
    st = original_trace(m, spec.get("addr", 0), len(spec["args"]),
                        want_state=spec.get("state"),
                        occurrence=args.occurrence,
                        overlay_off=spec.get("overlay"),
                        driver_state=spec.get("driver_state"),
                        want_planes=spec.get("planes", False),
                        reg_args=spec.get("regs"),
                        src_from=spec.get("src_from"),
                        src_stack=spec.get("src_stack"),
                        budget=spec.get("budget", 40_000_000))

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
        print("%s: ENTERED but the return was never detected." % args.routine)
        print("  entry SP=%#06x expecting return to %04x:%04x"
              % (st["sp"], st["ret"][1], st["ret"][0]))
        print("  %d hardware events recorded before the budget ran out"
              % len(st["events"]))
        return 2

    if spec.get("overlay") is not None:
        print("%s at VM.OVL VGA:0x%04x (loaded at segment %04x)"
              % (args.routine, spec["overlay"], st["drv_seg"]))
    else:
        print("%s at 0x%05x" % (args.routine, spec["addr"]))
    names = spec.get("regs") or [n for n, _ in spec["args"]]
    print("  arguments: %s"
          % ", ".join("%s=%#06x" % (n, v) for n, v in zip(names, st["args"])))

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

    for name, off, size in spec.get("driver_state", []):
        v = st["drv"][off]
        if size == 1:
            ctypes.c_uint8.in_dll(lib, name).value = v
            print("  driver   : %s = %#04x (VGA:DS %#06x)" % (name, v, off))
        elif size > 2:
            # An array - the row table, for one. Copied in whole rather than
            # rebuilt, because the driver set-up that fills it is not
            # transcribed yet and inventing it would be writing our own.
            buf = (ctypes.c_ubyte * size).in_dll(lib, name)
            ctypes.memmove(buf, v, size)
            print("  driver   : %s = %d bytes (VGA:DS %#06x)" % (name, size, off))
        else:
            ctypes.c_uint16.in_dll(lib, name).value = v
            print("  driver   : %s = %#06x (VGA:DS %#06x)" % (name, v, off))

    def seed(l):
        if st["gc_in"] is not None:
            g = (ctypes.c_ubyte * 9).from_buffer_copy(st["gc_in"])
            l.vga_load_regs(g, ctypes.c_ubyte(st["mask_in"]))
        if st["planes_in"] is not None:
            for i, pl in enumerate(st["planes_in"]):
                buf = (ctypes.c_ubyte * len(pl)).from_buffer_copy(pl)
                l.vga_load_plane(ctypes.c_int32(i), buf, ctypes.c_int32(len(pl)))

    if st["planes_in"] is not None:
        print("  planes   : seeding 4 x %d bytes from the original"
              % len(st["planes_in"][0]))

    lib.frame_pending.restype = ctypes.c_int16
    call_args = list(st["args"])
    if st["src"] is not None:
        call_args.append((ctypes.c_ubyte * len(st["src"])).from_buffer_copy(st["src"]))
    got_all = port_trace(lib, lambda l: spec["call"](l, call_args), setup=seed)
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
        rv = spec["call"](lib, call_args)
        want_ax = st["ax"] & 0xFFFF
        got_ax = rv & 0xFFFF
        print("  return   : original AX=%#06x  port=%#06x  %s"
              % (want_ax, got_ax, "ok" if want_ax == got_ax else "DIFFERS"))
        if want_ax != got_ax:
            return 1

    if st["state_out"] is not None:
        bad_state = 0
        for name, off, size in spec["state"]:
            want_v = st["state_out"][off]
            sym = (ctypes.c_uint8 if size == 1 else ctypes.c_int16)
            got_v = sym.in_dll(lib, name).value & ((1 << (8 * size)) - 1)
            mark = "ok" if want_v == got_v else "DIFFERS"
            if want_v != got_v:
                bad_state += 1
                bad += 1
            print("  after    : %-18s original %#06x  port %#06x  %s"
                  % (name, want_v, got_v, mark))

    if st["planes_out"] is not None:
        # The strongest check available: after both have run, the video memory
        # itself must match, not merely the sequence of writes.
        diff = 0
        for i, want_pl in enumerate(st["planes_out"]):
            buf = (ctypes.c_ubyte * len(want_pl))()
            lib.vga_store_plane(ctypes.c_int32(i), buf, ctypes.c_int32(len(want_pl)))
            got_pl = bytes(buf)
            diff += sum(1 for a, b in zip(want_pl, got_pl) if a != b)
        print("  planes   : %d of %d bytes differ after the call"
              % (diff, 4 * len(st["planes_out"][0])))
        if diff:
            bad += diff

    if bad == 0:
        print("  AGREED: %d events identical" % len(want))
        if not want and not spec.get("returns") and st["state_out"] is None:
            print("  NOTE: nothing was written and there is no return value to "
                  "compare - this call did no work, so an agreement here is "
                  "not evidence")
        return 0
    print("  DIFFERS in %d of %d events" % (bad, n))
    return 1


STATUS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      "STATUS.md")
BEGIN, END = "<!-- VERIFY:BEGIN -->", "<!-- VERIFY:END -->"


def sweep():
    """Verify every routine and write the result into STATUS.md."""
    import subprocess
    rows = []
    for name, spec in ROUTINES.items():
        where = ("VM.OVL VGA:0x%04x" % spec["overlay"]) if spec.get("overlay") \
            else ("0x%05x" % spec["addr"])
        occ = spec.get("check_occurrences", [0])
        results = []
        for o in occ:
            r = subprocess.run([sys.executable, __file__, name,
                                "--occurrence", str(o)],
                               capture_output=True, text=True)
            out = r.stdout
            if "AGREED" in out and r.returncode == 0:
                nev = [l for l in out.split("\n") if "AGREED" in l][0].strip()
                results.append((o, "agreed", nev))
            elif "NOT ENTERED" in out:
                results.append((o, "NOT REACHED", "never called"))
            else:
                results.append((o, "DIFFERS", out.strip().split("\n")[-1]))
        ok = all(x[1] == "agreed" for x in results)
        rows.append((name, where, ok, results))
        print("%-24s %-22s %s" % (name, where,
                                  "verified" if ok else "NOT VERIFIED"))

    lines = ["| routine | address | occurrences checked | result |",
             "| --- | --- | --- | --- |"]
    for name, where, ok, results in rows:
        lines.append("| `%s` | %s | %s | %s |"
                     % (name, where,
                        ", ".join(str(o) for o, _, _ in results),
                        "agreed" if ok else "**not verified**"))
    nver = sum(1 for r in rows if r[2])
    lines.append("")
    lines.append("*%d transcribed, %d verified. Written by "
                 "`tools/verify.py --all`, not by hand.*" % (len(rows), nver))
    table = "\n".join(lines)

    if os.path.exists(STATUS):
        txt = open(STATUS).read()
        if BEGIN in txt and END in txt:
            pre = txt[:txt.index(BEGIN) + len(BEGIN)]
            post = txt[txt.index(END):]
            open(STATUS, "w").write(pre + "\n" + table + "\n" + post)
            print("\nwrote the table into STATUS.md")
        else:
            print("\n(STATUS.md has no VERIFY markers; table not written)")
            print(table)
    return 0 if all(r[2] for r in rows) else 1


def fmt(e):
    if e is None:
        return "-"
    port, off, val, rd = e
    if port == 0xA000:
        return "A000:%04x %s %02x" % (off, "read " if rd else "write", val)
    return "%s %#05x = %02x" % ("in " if rd else "out", port, val)


if __name__ == "__main__":
    sys.exit(main())
