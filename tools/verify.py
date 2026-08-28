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
    "vm_save_rect": dict(
        overlay=0x12FB,
        planes=True,
        args=[("buf_off", 4), ("buf_seg", 6), ("x", 8), ("y", 10),
              ("w", 12), ("h", 14)],
        check_occurrences=[0],
        call=lambda lib, a: lib.vm_save_rect(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]),
            *[ctypes.c_int16(v) for v in a[2:]]),
    ),
    "vm_restore_rect": dict(
        overlay=0x13B9,
        planes=True,
        args=[("buf_off", 4), ("buf_seg", 6), ("x", 8), ("y", 10),
              ("w", 12), ("h", 14)],
        check_occurrences=[0],
        call=lambda lib, a: lib.vm_restore_rect(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]),
            *[ctypes.c_int16(v) for v in a[2:]]),
    ),
    "atan2_long": dict(
        addr=0x2D296,
        args=[("a_lo", 4), ("a_hi", 6), ("b_lo", 8), ("b_hi", 10)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.atan2_long(*[ctypes.c_uint16(v) for v in a]),
    ),
    "link_nearby_objects": dict(
        addr=0x03566,
        args=[("obj", 4), ("flags", 6), ("margin_x0", 8), ("margin_x1", 10),
              ("margin_y0", 12), ("margin_y1", 14)],
        check_occurrences=[0],
        call=lambda lib, a: lib.link_nearby_objects(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]),
            *[ctypes.c_int16(v) for v in a[2:]]),
    ),
    "find_edge_contact_reversed": dict(
        addr=0x00B6C,
        args=[("test_only", 4)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.find_edge_contact_reversed(
            ctypes.c_int16(a[0])),
    ),
    "resolve_collisions": dict(
        addr=0x00556,
        args=[("obj", 4)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.resolve_collisions(ctypes.c_uint16(a[0])),
    ),
    "find_edge_contact": dict(
        addr=0x007AF,
        args=[("test_only", 4)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.find_edge_contact(ctypes.c_int16(a[0])),
    ),
    "integrate_object": dict(
        addr=0x02C93,
        args=[("obj", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.integrate_object(ctypes.c_uint16(a[0])),
    ),
    "place_object_for_draw": dict(
        addr=0x05BE4,
        args=[("obj", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.place_object_for_draw(ctypes.c_uint16(a[0])),
    ),
    "add_sub_object_shapes": dict(
        addr=0x05EF6,
        args=[("obj", 4), ("mask", 6)],
        check_occurrences=[0],
        call=lambda lib, a: lib.add_sub_object_shapes(
            ctypes.c_uint16(a[0]), ctypes.c_int16(a[1])),
    ),
    "set_object_extent": dict(
        addr=0x05C77,
        args=[("obj", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.set_object_extent(ctypes.c_uint16(a[0])),
    ),
    "object_delta_angle": dict(
        addr=0x004AB,
        args=[("obj", 4)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.object_delta_angle(ctypes.c_uint16(a[0])),
    ),
    "arctan_lookup": dict(
        addr=0x2A941,
        args=[("index", 4)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.arctan_lookup(ctypes.c_uint16(a[0])),
    ),
    "apply_contact_friction": dict(
        addr=0x02DA0,
        args=[("obj", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.apply_contact_friction(ctypes.c_uint16(a[0])),
    ),
    "vm_read_pixel": dict(
        overlay=0x1453,
        planes=True,
        returns=True,
        args=[("x", 4), ("y", 6)],
        check_occurrences=[0],
        call=lambda lib, a: lib.vm_read_pixel(
            ctypes.c_int16(a[0]), ctypes.c_int16(a[1])),
    ),
    "read_pixel_clipped": dict(
        addr=0x2241B,
        planes=True,
        returns=True,
        args=[("x", 4), ("y", 6)],
        check_occurrences=[0],
        call=lambda lib, a: lib.read_pixel_clipped(
            ctypes.c_int16(a[0]), ctypes.c_int16(a[1])),
    ),
    "vm_plot_pixel": dict(
        overlay=0x14C9,
        planes=True,
        returns=True,
        args=[("x", 4), ("y", 6), ("colour", 8)],
        check_occurrences=[0],
        call=lambda lib, a: lib.vm_plot_pixel(
            ctypes.c_int16(a[0]), ctypes.c_int16(a[1]), ctypes.c_uint8(a[2])),
    ),
    "plot_pixel_clipped": dict(
        addr=0x2244D,
        planes=True,
        returns=True,
        args=[("x", 4), ("y", 6), ("colour", 8)],
        check_occurrences=[0],
        call=lambda lib, a: lib.plot_pixel_clipped(
            *[ctypes.c_int16(v) for v in a]),
    ),
    # The sound driver. Arguments arrive in registers - the AIL convention -
    # and these are near calls within the driver, not entries through its
    # dispatcher, so the port's functions take them as ordinary parameters.
    "init_sequence_params": dict(
        addr=0x28305,
        args=[],
        regs=["es", "ax"],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.init_sequence_params(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "next_matching_record": dict(
        addr=0x29966,
        args=[("selector", 4)],
        returns_pair=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: _next_matching_record(lib, a),
    ),
    "midi_bend_event": dict(
        addr=0x280FE,
        args=[],
        regs=["ds", "bp", "es", "bx", "si", "ax"],
        near=True,
        returns_in=("bp", 0xFFFF),
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.midi_bend_event(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "midi_note_event": dict(
        addr=0x27EE1,
        args=[],
        regs=["ds", "bp", "es", "bx", "si", "ax"],
        near=True,
        returns_in=("bp", 0xFFFF),
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.midi_note_event(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "free_node_list": dict(
        addr=0x28BAF,
        args=[("off", 4), ("seg", 6)],
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.free_node_list(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "create_sequence": dict(
        addr=0x28935,
        args=[("src_off", 4), ("src_seg", 6)],
        returns_pair=True,
        check_occurrences=[0],
        call=lambda lib, a: _create_sequence(lib, a),
    ),
    "free_for_kind": dict(
        addr=0x2A017,
        args=[("off", 4), ("seg", 6), ("kind", 8)],
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.free_for_kind(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "alloc_for_kind": dict(
        addr=0x29F89,
        args=[("size_lo", 4), ("size_hi", 6), ("kind", 8)],
        returns_pair=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: _alloc_for_kind(lib, a),
    ),
    "poll_sequences": dict(
        addr=0x27B7E,
        args=[],
        regs=[],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.poll_sequences(),
    ),
    "remove_sequence": dict(
        addr=0x26E7B,
        args=[],
        regs=["es", "ax"],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.remove_sequence(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "sound_callback": dict(
        addr=0x292A1,
        args=[],
        regs=["ax"],
        returns_in=("ax", 0xFFFF),
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.sound_callback(ctypes.c_uint16(a[0])),
    ),
    "sequencer_tick": dict(
        addr=0x26F2A,
        args=[],
        regs=[],
        near=True,
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: lib.sequencer_tick(),
    ),
    "flush_pending_volumes": dict(
        addr=0x27A86,
        args=[],
        regs=[],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.flush_pending_volumes(),
    ),
    "sx_controller": dict(
        sx_overlay=0x03A1,
        args=[],
        regs=["ax", "cx"],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.sx_controller(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "sx_pitch_bend": dict(
        sx_overlay=0x0410,
        args=[],
        regs=["ax", "cx"],
        near=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.sx_pitch_bend(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "sx_stop_note": dict(
        sx_overlay=0x037B,
        args=[],
        regs=["cx"],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.sx_stop_note(ctypes.c_uint16(a[0])),
    ),
    "sx_start_note": dict(
        sx_overlay=0x0386,
        args=[],
        regs=["ax", "cx"],
        near=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: lib.sx_start_note(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "sx_speaker_off": dict(
        sx_overlay=0x0480,
        args=[],
        regs=[],
        near=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.sx_speaker_off(),
    ),
    "sx_apply_bend": dict(
        sx_overlay=0x04FD,
        args=[],
        regs=["bx"],
        near=True,
        returns_in=("bx", 0xFFFF),
        check_occurrences=[0],
        call=lambda lib, a: lib.sx_apply_bend(ctypes.c_uint16(a[0])),
    ),
    "sx_note_on": dict(
        sx_overlay=0x0497,
        args=[],
        regs=["bx"],
        near=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.sx_note_on(ctypes.c_uint16(a[0])),
    ),
    "vm_buffer_size": dict(
        overlay=0x138E,
        args=[("w", 4), ("h", 6)],
        returns_pair=True,
        check_occurrences=[0, 1],
        call=lambda lib, a: _vm_buffer_size(lib, a),
    ),
    "vm_show_page": dict(
        overlay=0x150F,
        check_occurrences=[0, 3, 9],
        args=[("wait_retrace", 4)],
        call=lambda lib, a: lib.vm_show_page(ctypes.c_uint16(a[0])),
    ),
    "vm_copy_rect": dict(
        overlay=0x1561,
        planes=True,
        args=[("x", 4), ("y", 6), ("width", 8), ("height", 10)],
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
        # No buffer is handed over any more: the span list lives in the
        # guest's address space, which the port models, so ES:SI is enough.
        planes=True,
        # 300 needs a bigger budget than the default: the routine is called
        # 1,078 times in all, but not that often in the first 40M
        # instructions. The sweep reported it NOT ENTERED rather than passing
        # it, which is the distinction that makes an unchecked routine
        # visible.
        check_occurrences=[0, 1, 40, 300],
        budget=150_000_000,
        call=lambda lib, a: lib.vm_fill_spans(ctypes.c_uint16(a[0]),
                                              ctypes.c_uint16(a[1])),
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
        check_occurrences=[0, 5, 20],
        call=lambda lib, a: lib.present_frame(ctypes.c_uint16(a[0])),
    ),
    # The game's clipped rectangle fill. It needs the drawing state it reads
    # from DGROUP, and the driver state its span fill uses.
    "fill_rect": dict(
        addr=0x20079,
        args=[("x", 4), ("y", 6), ("w", 8), ("h", 10)],
        planes=True,
        check_occurrences=[0, 3, 60, 900],
        budget=150_000_000,
        call=lambda lib, a: lib.fill_rect(*[ctypes.c_int16(
            v if v < 0x8000 else v - 0x10000) for v in a]),
    ),
    "step_word_4e87": dict(
        addr=0x0144E,
        args=[],
        check_occurrences=[0, 5, 60],
        call=lambda lib, a: lib.step_word_4e87(),
    ),
    "set_clip_full_screen": dict(
        addr=0x0834B,
        args=[],
        # One occurrence, deliberately. The routine takes no arguments and has
        # no branches - it stores four constants - so a second call cannot
        # reach anything the first did not. It is also called only four times
        # in 200M instructions, so a later occurrence costs a long run for no
        # extra coverage.
        check_occurrences=[0],
        call=lambda lib, a: lib.set_clip_full_screen(),
    ),
    "sub_002be": dict(
        addr=0x002BE,
        args=[],
        check_occurrences=[0, 3, 12],
        call=lambda lib, a: lib.sub_002be(),
    ),
    "clear_word_array_50bf": dict(
        addr=0x166D6,
        args=[],
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: lib.clear_word_array_50bf(),
    ),
    "bit0_of_468c": dict(
        addr=0x2147D,
        args=[("index", 4)],
        returns=True,
        check_occurrences=[0, 4, 25],
        call=lambda lib, a: lib.bit0_of_468c(ctypes.c_uint16(a[0])),
    ),
    "advance_record": dict(
        addr=0x2891A,
        args=[("off", 4), ("seg", 6)],
        src_stack_far=(0, 1, 512),
        returns=True,
        # Called a handful of times only. Occurrence numbers are relative to
        # the sweep's own run - see STATUS.md - so the last one is not a safe
        # choice; these two are.
        check_occurrences=[0, 2],
        budget=200_000_000,
        call=lambda lib, a: lib.advance_record(a[2], ctypes.c_uint16(a[0])),
    ),
    "match_field_5a_5c": dict(
        addr=0x06F43,
        args=[("value", 4), ("obj", 6)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.match_field_5a_5c(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000),
            ctypes.c_uint16(a[1])),
    ),
    "lookup_table_546c": dict(
        addr=0x11D44,
        args=[("index", 4)],
        returns=True,
        check_occurrences=[0, 5, 30],
        call=lambda lib, a: lib.lookup_table_546c(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000)),
    ),
    # A near function: `ret`, not `retf`.
    "string_contains_r": dict(
        addr=0x1C6E3,
        near=True,
        args=[("str", 4)],
        returns=True,
        check_occurrences=[0, 2],
        budget=200_000_000,
        call=lambda lib, a: lib.string_contains_r(ctypes.c_uint16(a[0])),
    ),
    "flag_bit_48ea": dict(
        addr=0x2213E,
        args=[("which", 4)],
        returns=True,
        check_occurrences=[0, 4, 30],
        call=lambda lib, a: lib.flag_bit_48ea(ctypes.c_uint16(a[0])),
    ),
    "select_field_2_or_4": dict(
        addr=0x06F68,
        args=[("key", 4), ("rec", 6)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.select_field_2_or_4(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000),
            ctypes.c_uint16(a[1])),
    ),
    "read_pair_4740": dict(
        addr=0x220E9,
        args=[("out_a", 4), ("out_b", 6)],
        check_occurrences=[0, 2, 15],
        call=lambda lib, a: lib.read_pair_4740(ctypes.c_uint16(a[0]),
                                               ctypes.c_uint16(a[1])),
    ),
    # These take their argument with `mov bx, sp` and never set up BP, so it
    # still sits where a far function's first argument does.
    "angle_sin": dict(
        addr=0x2A456,
        args=[("angle", 4)],
        returns=True,
        check_occurrences=[0, 4, 25],
        call=lambda lib, a: lib.angle_sin(ctypes.c_uint16(a[0])),
    ),
    "angle_cos": dict(
        addr=0x2A47B,
        args=[("angle", 4)],
        returns=True,
        check_occurrences=[0, 4, 25],
        call=lambda lib, a: lib.angle_cos(ctypes.c_uint16(a[0])),
    ),
    "angle_to_quadrant": dict(
        addr=0x004D1,
        args=[("angle", 4)],
        returns=True,
        check_occurrences=[0, 5, 40],
        call=lambda lib, a: lib.angle_to_quadrant(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000)),
    ),
    "chain_contains": dict(
        addr=0x03A61,
        args=[("rec", 4), ("node", 6)],
        returns=True,
        check_occurrences=[0, 3, 25],
        call=lambda lib, a: lib.chain_contains(ctypes.c_uint16(a[0]),
                                               ctypes.c_uint16(a[1])),
    ),
    # A near routine that takes and answers registers.
    "normalise_far_ptr": dict(
        addr=0x22161,
        near=True,
        args=[],
        regs=["ax", "dx"],
        returns_pair=True,
        check_occurrences=[0, 4, 30],
        call=lambda lib, a: _normalise_far_ptr(lib, a),
    ),
    "follow_far_chain": dict(
        addr=0x2907B,
        args=[("off", 4), ("seg", 6), ("count", 8)],
        returns_pair=True,
        # Called only twice while the intro screens run, so this is all of it.
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: _follow_far_chain(lib, a),
    ),
    "step_pair_apart": dict(
        addr=0x03D2E,
        args=[("rec", 4)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.step_pair_apart(ctypes.c_uint16(a[0])),
    ),
    "points_within_140": dict(
        addr=0x04B53,
        args=[("a", 4), ("b", 6)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.points_within_140(ctypes.c_uint16(a[0]),
                                                  ctypes.c_uint16(a[1])),
    ),
    "splice_list_4e58_onto_4e56": dict(
        addr=0x07B3E,
        args=[],
        check_occurrences=[0, 2, 10],
        budget=200_000_000,
        call=lambda lib, a: lib.splice_list_4e58_onto_4e56(),
    ),
    # A near routine taking and answering CL, with DL as its second input.
    "scale_byte_pair": dict(
        addr=0x282CB,
        near=True,
        args=[],
        regs=["cx", "dx"],
        # The result is CL, not AX: the routine pushes and pops AX around its
        # own multiply. Called only twice on these screens.
        returns_in=("cx", 0xFF),
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: lib.scale_byte_pair(ctypes.c_uint8(a[0] & 0xFF),
                                                ctypes.c_uint8(a[1] & 0xFF)),
    ),
    "value_between": dict(
        addr=0x03D67,
        args=[("v", 4), ("a", 6), ("b", 8)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.value_between(*[ctypes.c_uint16(v) for v in a]),
    ),
    "pick_by_flag": dict(
        addr=0x05B65,
        args=[("flags", 4)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.pick_by_flag(ctypes.c_uint16(a[0])),
    ),
    "normalise_far_ptr_far": dict(
        addr=0x22386,
        args=[("frac", 4), ("whole", 6)],
        returns_pair=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: _normalise_far_ptr_far(lib, a),
    ),
    "compute_bounds_53fe": dict(
        addr=0x00386,
        args=[],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.compute_bounds_53fe(),
    ),
    "pick_for_record": dict(
        addr=0x05BA7,
        args=[("rec", 4), ("flags", 6)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.pick_for_record(ctypes.c_uint16(a[0]),
                                                ctypes.c_uint16(a[1])),
    ),
    "set_side_flags": dict(
        addr=0x004FD,
        args=[("range", 4), ("v", 6), ("out", 8)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.set_side_flags(
            ctypes.c_uint16(a[0]),
            ctypes.c_int16(a[1] if a[1] < 0x8000 else a[1] - 0x10000),
            ctypes.c_uint16(a[2])),
    ),
    # NOT VERIFIABLE by this harness, and skipped rather than reported as
    # agreeing. Its whole purpose is to wait for the INT 08h handler to set a
    # flag, and the harness suppresses interrupts while a routine is open so
    # that an interrupt's own hardware writes are not counted as the routine's.
    # With them suppressed the original's spin can never be released and the
    # emulator sits in it forever. Verifying it would need the harness to
    # distinguish an interrupt's effects from the routine's rather than
    # excluding them, which it cannot do today.
    "wait_and_latch_frame": dict(
        addr=0x0AACA,
        args=[],
        unverifiable="waits for an interrupt the harness must suppress",
        check_occurrences=[],
        call=lambda lib, a: lib.wait_and_latch_frame(),
    ),
    "far_memcpy": dict(
        addr=0x222C6,
        args=[("dst_off", 4), ("dst_seg", 6), ("src_off", 8),
              ("src_seg", 10), ("count", 12)],
        check_occurrences=[0, 2],
        call=lambda lib, a: lib.far_memcpy(*[ctypes.c_uint16(v) for v in a]),
    ),
    "claim_page_slot": dict(
        addr=0x0B429,
        args=[("want", 4)],
        returns=True,
        # Called about a dozen times on these screens.
        check_occurrences=[0, 3, 9],
        call=lambda lib, a: lib.claim_page_slot(ctypes.c_uint16(a[0])),
    ),
    "save_or_restore_draw_state": dict(
        addr=0x0B47F,
        args=[("save", 4)],
        check_occurrences=[0, 1, 8],
        call=lambda lib, a: lib.save_or_restore_draw_state(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000)),
    ),
    "clamp_record_pair": dict(
        addr=0x02BCC,
        args=[("rec", 4)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.clamp_record_pair(ctypes.c_uint16(a[0])),
    ),
    "set_clip_for_mode": dict(
        addr=0x082C3,
        args=[],
        check_occurrences=[0, 2, 8],
        call=lambda lib, a: lib.set_clip_for_mode(),
    ),
    "link_record_into_buckets": dict(
        addr=0x166EF,
        args=[("rec", 4)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.link_record_into_buckets(ctypes.c_uint16(a[0])),
    ),
    "update_velocity": dict(
        addr=0x07283,
        args=[("rec", 4), ("shift_x", 6), ("shift_y", 8), ("which", 10)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.update_velocity(
            ctypes.c_uint16(a[0]), ctypes.c_uint8(a[1] & 0xFF),
            ctypes.c_uint8(a[2] & 0xFF), ctypes.c_uint16(a[3])),
    ),
    "clip_and_draw_line": dict(
        addr=0x21E34,
        args=[("x1", 4), ("y1", 6), ("x2", 8), ("y2", 10)],
        planes=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.clip_and_draw_line(*[
            ctypes.c_int16(v if v < 0x8000 else v - 0x10000) for v in a]),
    ),
    # Register arguments: BX,CX to DX,SI, destination page in ES.
    "vm_draw_line": dict(
        overlay=0x0998,
        args=[],
        regs=["bx", "cx", "dx", "si"],
        planes=True,
        check_occurrences=[0, 2, 9, 30],
        call=lambda lib, a: lib.vm_draw_line(*[
            ctypes.c_int16(v if v < 0x8000 else v - 0x10000) for v in a]),
    ),
    "far_memset": dict(
        addr=0x22300,
        args=[("off", 4), ("seg", 6), ("value", 8), ("count_lo", 10),
              ("count_hi", 12)],
        check_occurrences=[0, 2, 9],
        call=lambda lib, a: lib.far_memset(*[ctypes.c_uint16(v) for v in a]),
    ),
    # NOT VERIFIABLE for the same reason as wait_and_latch_frame, which it
    # calls: the harness suppresses interrupts while a routine is open, and
    # that wait can then never end. Marked up front rather than left for the
    # watchdog to discover.
    "update_button_state": dict(
        addr=0x08136,
        args=[],
        unverifiable="calls wait_and_latch_frame, which waits for an interrupt",
        check_occurrences=[],
        call=lambda lib, a: lib.update_button_state(),
    ),
    "compute_swept_bounds_5400": dict(
        addr=0x002DD,
        args=[],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.compute_swept_bounds_5400(),
    ),
    "angles_same_side": dict(
        addr=0x003DF,
        args=[("angle", 4)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.angles_same_side(
            ctypes.c_int16(a[0] if a[0] < 0x8000 else a[0] - 0x10000)),
    ),
    "insert_sorted": dict(
        addr=0x05646,
        args=[("rec", 4), ("head", 6)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.insert_sorted(ctypes.c_uint16(a[0]),
                                              ctypes.c_uint16(a[1])),
    ),
    "dos_alloc_bytes": dict(
        addr=0x21ABD,
        args=[("size_lo", 4), ("size_hi", 6), ("unused", 8), ("flags", 10)],
        returns_pair=True,
        check_occurrences=[0, 2, 9],
        call=lambda lib, a: _dos_alloc_bytes(lib, a),
    ),
    "mul16x16": dict(
        addr=0x2A269,
        args=[("a", 4), ("b", 6)],
        returns_pair=True,
        check_occurrences=[0, 5, 40],
        call=lambda lib, a: _mul16x16(lib, a),
    ),
    "apply_gravity_and_speed": dict(
        addr=0x02C39,
        args=[("rec", 4)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.apply_gravity_and_speed(ctypes.c_uint16(a[0])),
    ),
    "vm_load_palette": dict(
        overlay=0x0F15,
        args=[("off", 4), ("seg", 6)],
        check_occurrences=[0, 1, 2],
        call=lambda lib, a: lib.vm_load_palette(ctypes.c_uint16(a[0]),
                                                ctypes.c_uint16(a[1])),
    ),
    "set_palette_pointer": dict(
        addr=0x1EB6A,
        args=[("off", 4), ("seg", 6)],
        returns_pair=True,
        check_occurrences=[0, 1, 2],
        call=lambda lib, a: _set_palette_pointer(lib, a),
    ),
    "rotate_point": dict(
        addr=0x03B17,
        args=[("px", 4), ("py", 6), ("angle", 8)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.rotate_point(*[ctypes.c_uint16(v) for v in a]),
    ),
    "alloc_shape": dict(
        addr=0x064B4,
        args=[("pt1", 4), ("pt2", 6), ("flags", 8), ("which", 10),
              ("width", 12)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.alloc_shape(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]),
            ctypes.c_uint8(a[2] & 0xFF), ctypes.c_uint8(a[3] & 0xFF),
            ctypes.c_int16(a[4] if a[4] < 0x8000 else a[4] - 0x10000)),
    ),
    "add_record_shapes": dict(
        addr=0x0642A,
        args=[("rec", 4), ("which", 6)],
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.add_record_shapes(ctypes.c_uint16(a[0]),
                                                  ctypes.c_uint16(a[1])),
    ),
    "recompute_kind_physics": dict(
        addr=0x02AC0,
        args=[],
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: lib.recompute_kind_physics(),
    ),
    "reset_input_state": dict(
        addr=0x0B4F1,
        args=[],
        check_occurrences=[0, 1],
        budget=200_000_000,
        call=lambda lib, a: lib.reset_input_state(),
    ),
    "compute_link_endpoints": dict(
        addr=0x04E65,
        args=[("link", 4)],
        # Called a handful of times on these screens.
        check_occurrences=[0, 3, 6],
        call=lambda lib, a: lib.compute_link_endpoints(ctypes.c_uint16(a[0])),
    ),
    "find_entry_for_pointer": dict(
        addr=0x098E0,
        args=[("out", 4)],
        returns=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.find_entry_for_pointer(ctypes.c_uint16(a[0])),
    ),
    "erase_both_pages": dict(
        addr=0x080E7,
        planes=True,
        args=[],
        check_occurrences=[0],
        call=lambda lib, a: lib.erase_both_pages(),
    ),
    "erase_object": dict(
        addr=0x0AD51,
        planes=True,
        args=[("handle", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.erase_object(ctypes.c_uint16(a[0])),
    ),
    "restage_object_rect": dict(
        addr=0x0AEF6,
        args=[("handle", 4)],
        check_occurrences=[0],
        call=lambda lib, a: lib.restage_object_rect(ctypes.c_uint16(a[0])),
    ),
    "claim_buffer_slot": dict(
        addr=0x0B5ED,
        args=[("a_lo", 4), ("a_hi", 6), ("b_lo", 8), ("b_hi", 10)],
        returns=True,
        check_occurrences=[0],
        call=lambda lib, a: lib.claim_buffer_slot(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "clear_slot_5734": dict(
        addr=0x0B69C,
        args=[("n", 4)],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.clear_slot_5734(ctypes.c_int16(a[0])),
    ),
    "clear_flag_2d44": dict(
        addr=0x0A7A3,
        args=[],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.clear_flag_2d44(),
    ),
    "clear_flag_2d44_thunk": dict(
        addr=0x0811B,
        args=[],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.clear_flag_2d44_thunk(),
    ),
    "dos_free_far": dict(
        addr=0x21B34,
        args=[("off", 4), ("seg", 6)],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.dos_free_far(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1])),
    ),
    "refresh_link_geometry": dict(
        addr=0x04F7F,
        args=[("link", 4)],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.refresh_link_geometry(ctypes.c_uint16(a[0])),
    ),
    "set_vector_from_angle": dict(
        addr=0x07223,
        args=[("obj", 4), ("angle", 6), ("mag", 8)],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.set_vector_from_angle(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]), ctypes.c_int16(a[2])),
    ),
    "link_slack": dict(
        addr=0x0713D,
        args=[("obj", 4), ("link", 6), ("gen", 8)],
        returns=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.link_slack(
            ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]), ctypes.c_int16(a[2])),
    ),
    "link_endpoint_gap": dict(
        addr=0x07947,
        args=[("link", 4), ("obj", 6), ("out_dx", 8), ("out_dy", 10)],
        returns=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.link_endpoint_gap(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "link_end_distance": dict(
        addr=0x06F8E,
        args=[("link", 4), ("gen", 6), ("end", 8)],
        returns=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.link_end_distance(
            ctypes.c_uint16(a[0]), ctypes.c_int16(a[1]), ctypes.c_int16(a[2])),
    ),
    "shift_all_histories": dict(
        addr=0x07CA2,
        args=[],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.shift_all_histories(),
    ),
    "shift_state_history": dict(
        addr=0x07CE3,
        args=[("obj", 4)],
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.shift_state_history(ctypes.c_uint16(a[0])),
    ),
    "compare_link_ends": dict(
        addr=0x06DE9,
        args=[("link", 4), ("end", 6), ("reversed", 8)],
        returns=True,
        check_occurrences=[0, 1, 4],
        call=lambda lib, a: lib.compare_link_ends(
            ctypes.c_uint16(a[0]), ctypes.c_int16(a[1]), ctypes.c_int16(a[2])),
    ),
    "intersect_segments": dict(
        addr=0x03BA9,
        args=[("seg1", 4), ("seg2", 6), ("out", 8)],
        returns=True,
        check_occurrences=[0, 3, 20],
        call=lambda lib, a: lib.intersect_segments(
            *[ctypes.c_uint16(v) for v in a]),
    ),
    "frame_pending": dict(
        addr=0x0B4E2,
        check_occurrences=[0, 1],
        args=[],
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
    lib.bit0_of_468c.restype = ctypes.c_int16
    lib.advance_record.restype = ctypes.c_uint16
    for fn in ("match_field_5a_5c", "lookup_table_546c",
               "string_contains_r", "flag_bit_48ea",
               "select_field_2_or_4", "angle_sin", "angle_cos"):
        getattr(lib, fn).restype = ctypes.c_int16
    lib.angle_to_quadrant.restype = ctypes.c_int16
    lib.chain_contains.restype = ctypes.c_int16
    lib.follow_far_chain.restype = ctypes.c_uint32
    lib.points_within_140.restype = ctypes.c_int16
    lib.scale_byte_pair.restype = ctypes.c_uint8
    lib.value_between.restype = ctypes.c_int16
    lib.pick_by_flag.restype = ctypes.c_int16
    lib.pick_for_record.restype = ctypes.c_int16
    lib.claim_page_slot.restype = ctypes.c_uint16
    lib.angles_same_side.restype = ctypes.c_int16
    lib.intersect_segments.restype = ctypes.c_int16
    lib.compare_link_ends.restype = ctypes.c_int16
    lib.find_entry_for_pointer.restype = ctypes.c_int16
    lib.link_end_distance.restype = ctypes.c_int16
    lib.link_endpoint_gap.restype = ctypes.c_int16
    lib.link_slack.restype = ctypes.c_int16
    lib.vm_buffer_size.restype = ctypes.c_uint32
    lib.sx_apply_bend.restype = ctypes.c_uint16
    lib.midi_note_event.restype = ctypes.c_uint16
    lib.midi_bend_event.restype = ctypes.c_uint16
    lib.next_matching_record.restype = ctypes.c_uint32
    lib.alloc_for_kind.restype = ctypes.c_uint32
    lib.create_sequence.restype = ctypes.c_uint32
    lib.sound_callback.restype = ctypes.c_uint16
    lib.vm_plot_pixel.restype = ctypes.c_uint16
    lib.vm_read_pixel.restype = ctypes.c_uint16
    lib.arctan_lookup.restype = ctypes.c_int16
    lib.atan2_long.restype = ctypes.c_int16
    lib.object_delta_angle.restype = ctypes.c_int16
    lib.find_edge_contact.restype = ctypes.c_int16
    lib.resolve_collisions.restype = ctypes.c_int16
    lib.find_edge_contact_reversed.restype = ctypes.c_int16
    lib.read_pixel_clipped.restype = ctypes.c_int16
    lib.plot_pixel_clipped.restype = ctypes.c_int16
    lib.claim_buffer_slot.restype = ctypes.c_int16
    lib.dos_alloc_bytes.restype = ctypes.c_uint32
    lib.mul16x16.restype = ctypes.c_uint32
    lib.set_palette_pointer.restype = ctypes.c_uint32
    lib.normalise_far_ptr_far.restype = ctypes.c_uint32
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




def _normalise_far_ptr(lib, a):
    off = ctypes.c_uint16(a[0])
    seg = ctypes.c_uint16(a[1])
    lib.normalise_far_ptr(ctypes.byref(off), ctypes.byref(seg))
    return off.value, seg.value


def _follow_far_chain(lib, a):
    r = lib.follow_far_chain(ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]),
                             ctypes.c_int16(a[2] if a[2] < 0x8000
                                            else a[2] - 0x10000))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _normalise_far_ptr_far(lib, a):
    r = lib.normalise_far_ptr_far(ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _dos_alloc_bytes(lib, a):
    r = lib.dos_alloc_bytes(*[ctypes.c_uint16(v) for v in a[:4]])
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _create_sequence(lib, a):
    r = lib.create_sequence(ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _alloc_for_kind(lib, a):
    r = lib.alloc_for_kind(*[ctypes.c_uint16(v) for v in a])
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _next_matching_record(lib, a):
    r = lib.next_matching_record(ctypes.c_int16(
        a[0] if a[0] < 0x8000 else a[0] - 0x10000))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _vm_buffer_size(lib, a):
    r = lib.vm_buffer_size(ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _mul16x16(lib, a):
    r = lib.mul16x16(*[ctypes.c_int16(v if v < 0x8000 else v - 0x10000)
                       for v in a[:2]])
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def _set_palette_pointer(lib, a):
    r = lib.set_palette_pointer(ctypes.c_uint16(a[0]), ctypes.c_uint16(a[1]))
    return r & 0xFFFF, (r >> 16) & 0xFFFF


def compare_instance(inst, lib, verbose=True):
    """Run the port on one captured call and compare. Returns (ok, summary)."""
    spec = inst["spec"]
    out = []

    def say(line):
        out.append(line)
        if verbose:
            print(line)

    names = spec.get("regs") or [n for n, _ in spec["args"]]
    say("  arguments: %s"
        % ", ".join("%s=%#06x" % (n, v) for n, v in zip(names, inst["args"])))

    # Nothing is seeded by name any more. The driver's data lives inside
    # DGROUP - see reconstruct/dgroup.h - so the whole segment carries it.

    def seed(l):
        allocs = inst.get("allocs") or []
        if allocs:
            n = len(allocs)
            segs = (ctypes.c_uint16 * n)(*[a[1] for a in allocs])
            large = (ctypes.c_uint16 * n)(*[a[2] for a in allocs])
            fail = (ctypes.c_ubyte * n)(*[1 if a[3] else 0 for a in allocs])
            l.io_prime_dos_alloc(segs, large, fail, ctypes.c_int32(n))
        if inst["mem_in"] is not None:
            ctypes.c_uint32.in_dll(l, "dgroup_base").value = inst["dg_base"]
            # The port's stand-in stack - see reconstruct/dgroup.h. Setting it
            # to the original's entry SP puts any frame the port reserves
            # inside the range excluded from the memory comparison below.
            ctypes.c_uint16.in_dll(l, "guest_sp").value = inst["sp"]
            gm = (ctypes.c_ubyte * 0x100000).in_dll(l, "guest_mem")
            ctypes.memmove(gm, inst["mem_in"], 0xA0000)
        if inst["gc_in"] is not None:
            g = (ctypes.c_ubyte * 9).from_buffer_copy(inst["gc_in"])
            l.vga_load_regs(g, ctypes.c_ubyte(inst["mask_in"]))
        if inst["planes_in"] is not None:
            for i, pl in enumerate(inst["planes_in"]):
                b = (ctypes.c_ubyte * len(pl)).from_buffer_copy(pl)
                l.vga_load_plane(ctypes.c_int32(i), b, ctypes.c_int32(len(pl)))

    call_args = list(inst["args"])
    if inst["src"] is not None:
        call_args.append(
            (ctypes.c_ubyte * len(inst["src"])).from_buffer_copy(inst["src"]))

    lib.frame_pending.restype = ctypes.c_int16
    lib.bit0_of_468c.restype = ctypes.c_int16
    lib.advance_record.restype = ctypes.c_uint16
    for fn in ("match_field_5a_5c", "lookup_table_546c",
               "string_contains_r", "flag_bit_48ea",
               "select_field_2_or_4", "angle_sin", "angle_cos"):
        getattr(lib, fn).restype = ctypes.c_int16
    lib.angle_to_quadrant.restype = ctypes.c_int16
    lib.chain_contains.restype = ctypes.c_int16
    lib.follow_far_chain.restype = ctypes.c_uint32
    lib.points_within_140.restype = ctypes.c_int16
    lib.scale_byte_pair.restype = ctypes.c_uint8
    lib.value_between.restype = ctypes.c_int16
    lib.pick_by_flag.restype = ctypes.c_int16
    lib.pick_for_record.restype = ctypes.c_int16
    lib.claim_page_slot.restype = ctypes.c_uint16
    lib.angles_same_side.restype = ctypes.c_int16
    lib.intersect_segments.restype = ctypes.c_int16
    lib.compare_link_ends.restype = ctypes.c_int16
    lib.find_entry_for_pointer.restype = ctypes.c_int16
    lib.link_end_distance.restype = ctypes.c_int16
    lib.link_endpoint_gap.restype = ctypes.c_int16
    lib.link_slack.restype = ctypes.c_int16
    lib.dos_alloc_bytes.restype = ctypes.c_uint32
    lib.mul16x16.restype = ctypes.c_uint32
    lib.set_palette_pointer.restype = ctypes.c_uint32
    lib.normalise_far_ptr_far.restype = ctypes.c_uint32
    got_all = port_trace(lib, lambda l: spec["call"](l, call_args), setup=seed)

    want = [e for e in inst["events"] if not e[3]]
    got = [e for e in got_all if not e[3]]
    say("  original : %d writes   port: %d writes" % (len(want), len(got)))

    bad = 0
    for i in range(max(len(want), len(got))):
        w = want[i] if i < len(want) else None
        g = got[i] if i < len(got) else None
        if w != g:
            bad += 1
            if bad <= 8:
                say("    %3d  original %s   port %s" % (i, fmt(w), fmt(g)))

    if spec.get("returns_in"):
        # Accept a bare register name as well as (name, mask). A plain string
        # used to unpack into two characters and fail with a type error a long
        # way from the spec that caused it.
        ri = spec["returns_in"]
        rname, mask = ri if isinstance(ri, tuple) else (ri, 0xFFFF)
        lib.io_reset()
        seed(lib)
        gv = spec["call"](lib, call_args) & mask
        wv = inst["regs_out"][rname] & mask
        say("  return %s: original %#06x  port %#06x  %s"
            % (rname.upper(), wv, gv, "ok" if wv == gv else "DIFFERS"))
        if wv != gv:
            bad += 1
    elif spec.get("returns") or spec.get("returns_pair"):
        lib.io_reset()
        seed(lib)
        rv = spec["call"](lib, call_args)
        if spec.get("returns_pair"):
            # Some routines answer in DX:AX - a far pointer, or a fixed-point
            # pair - so comparing AX alone would miss half the result.
            got_ax, got_dx = rv
            for label, wv, gv in (("AX", inst["ax"] & 0xFFFF, got_ax & 0xFFFF),
                                  ("DX", inst["dx"] & 0xFFFF, got_dx & 0xFFFF)):
                say("  return %s : original %#06x  port %#06x  %s"
                    % (label, wv, gv, "ok" if wv == gv else "DIFFERS"))
                if wv != gv:
                    bad += 1
        else:
            rv &= 0xFFFF
            wv = inst["ax"] & 0xFFFF
            say("  return   : original %#06x  port %#06x  %s"
                % (wv, rv, "ok" if wv == rv else "DIFFERS"))
            if wv != rv:
                bad += 1

    if inst["mem_out"] is not None:
        gm = bytes((ctypes.c_ubyte * 0x100000).in_dll(lib, "guest_mem"))[:0xA0000]
        want = inst["mem_out"]
        base_dg = inst["dg_base"]
        lo = base_dg + max(0, inst["sp_min"] - 8)
        # Up to and including the routine's own arguments. Several routines
        # here modify them in place - far_memset walks its 32-bit count down
        # with sub/sbb - and in cdecl the caller pops them, so those writes
        # cannot be observed by anyone. The port's arguments live in its own
        # frame and it has no way to reproduce them; excluding them is
        # correct rather than convenient.
        hi = (base_dg + inst["sp"] + inst["aoff"]
              + 2 * len(inst["spec"]["args"]))

        # VM.OVL's own code. The driver is **self-modifying** - VGA:0x0be6
        # patches the row-table pointer into cs:[0xbe4] and VGA:0x15d0 patches
        # an immediate at cs:[0x15ce] - and a C transcription has no code to
        # patch. This is the one class of difference the port cannot reproduce
        # and should not: excluded deliberately, not because it was awkward.
        ovl_lo = (inst["drv_seg"] or 0) * 16
        ovl_hi = ovl_lo + 0x2B10 if inst["drv_seg"] else 0

        diff, changed = [], []
        for i in range(0xA0000):
            if lo <= i < hi:
                continue          # bytes this call used as its stack
            if ovl_lo <= i < ovl_hi:
                continue          # the driver patching its own code
            if want[i] != gm[i]:
                diff.append(i)
            if want[i] != inst["mem_in"][i]:
                changed.append(i)
        say("  memory   : original changed %d bytes outside the stack "
            "(%#07x..%#07x); %d differ in the port"
            % (len(changed), lo, hi, len(diff)))
        for i in diff[:8]:
            say("    %#07x (DGROUP %#06x)  original %02x  port %02x"
                % (i, i - base_dg, want[i], gm[i]))
        bad += len(diff)

    if inst["planes_out"] is not None:
        diff = 0
        for i, wpl in enumerate(inst["planes_out"]):
            buf = (ctypes.c_ubyte * len(wpl))()
            lib.vga_store_plane(ctypes.c_int32(i), buf, ctypes.c_int32(len(wpl)))
            diff += sum(1 for a, b in zip(wpl, bytes(buf)) if a != b)
        say("  planes   : %d of %d bytes differ after the call"
            % (diff, 4 * len(inst["planes_out"][0])))
        bad += diff

    if bad == 0:
        say("  AGREED: %d events identical" % len(want))
        if not want and not spec.get("returns") \
                and not spec.get("returns_pair") \
                and not spec.get("returns_in") and not inst["mem_out"]:
            say("  NOTE: nothing written and nothing to compare - not evidence")
    else:
        say("  DIFFERS in %d places" % bad)
    return bad == 0, "\n".join(out)


def collect_all(names, budget=260_000_000):
    """Capture every wanted (routine, occurrence) in ONE run of the original.

    The per-routine path runs the game from the start for each check, which was
    fine at four routines and is fifteen minutes at twelve. This walks the game
    once and captures every routine's entry and exit as it goes.

    Routines **nest** - fill_rect calls the driver's span filler, present_frame
    calls the page flip - so several can be open at once and an outer routine's
    events must include the inner one's. Each open instance therefore keeps its
    own event list, its own entry SP and its own return address, and occurrence
    numbers still count every entry to that routine, so they mean the same
    thing they did when each was checked alone.
    """
    from unicorn import UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_READ, \
        UC_HOOK_MEM_WRITE
    import unicorn.x86_const as xc2

    m = drive.machine()
    base = m.load_seg * 16
    drv = {"seg": None}
    want = {}                      # name -> set of occurrences still wanted
    for n in names:
        want[n] = set(ROUTINES[n].get("check_occurrences", [0]))
    counts = {n: 0 for n in names}
    open_inst = []
    done = []
    addr_map = {"m": {}, "built": None}

    sx = {"seg": None, "dirty": True}

    def sx_seg():
        # The sound driver never writes to A000, so the trick that finds the
        # video driver cannot find it. The game holds a far pointer to it in
        # the sound module's own code segment, at 0x2619:0x1e7, and the loader
        # fills that in - so read it rather than hard-coding the segment.
        v = m.uc.mem_read(base + 0x26190 + 0x1e9, 2)
        seg = v[0] | (v[1] << 8)
        return seg or None

    def entry_addr(name):
        sp = ROUTINES[name]
        if sp.get("overlay") is not None:
            if drv["seg"] is None:
                return None
            return drv["seg"] * 16 + sp["overlay"]
        if sp.get("sx_overlay") is not None:
            seg = sx_seg()
            if seg is None:
                return None
            return seg * 16 + sp["sx_overlay"]
        return base + sp["addr"]

    def on_mem(uc, typ, address, size, value, ud):
        if drv["seg"] is None and typ == 17:
            cs = uc.reg_read(UC_X86_REG_CS)
            if not (base <= cs * 16 < base + DGROUP):
                drv["seg"] = cs
        if 0xA0000 <= address < 0xB0000:
            for inst in open_inst:
                inst["events"].append((0xA000, address - 0xA0000,
                                       value & 0xFF if typ == 17 else 0,
                                       0 if typ == 17 else 1))

    def on_out(uc, port, size, value, ud):
        for inst in open_inst:
            if size == 2:
                inst["events"].append((port, 0, value & 0xFF, 0))
                inst["events"].append((port + 1, 0, (value >> 8) & 0xFF, 0))
            else:
                inst["events"].append((port, 0, value & 0xFF, 0))

    def on_code(uc, address, size, ud):
        # Close any instance whose return address and stack have come back.
        if open_inst:
            cs = uc.reg_read(UC_X86_REG_CS)
            ip = uc.reg_read(UC_X86_REG_IP)
            sp = uc.reg_read(UC_X86_REG_SP)
            for inst in open_inst:
                # SS *is* DGROUP in this program, so the stack lives inside the
                # segment being compared. Track how deep each call went, so the
                # bytes it used as stack can be left out of the comparison -
                # the port has its own C stack and cannot reproduce them.
                if sp < inst["sp_min"]:
                    inst["sp_min"] = sp
            for inst in list(open_inst):
                if (ip, cs) == inst["ret"] and sp >= inst["sp"] + inst["aoff"]:
                    inst["ax"] = uc.reg_read(UC_X86_REG_AX)
                    inst["dx"] = uc.reg_read(UC_X86_REG_DX)
                    # Not every routine answers in AX. scale_byte_pair pushes
                    # and pops AX around its work and leaves its result in CL,
                    # so comparing AX compared a value the routine never
                    # touched - and reported a correct transcription as wrong.
                    inst["regs_out"] = {r: uc.reg_read(v)
                                        for r, v in REGS.items()
                                        if r not in ("flags",)}
                    if inst["spec"].get("planes"):
                        inst["planes_out"] = [bytes(p) for p in m.planes]
                    inst["mem_out"] = bytes(uc.mem_read(0, 0xA0000))
                    inst["allocs"] = m.dos_alloc_log[inst["alloc_from"]:]
                    inst["done"] = True
                    open_inst.remove(inst)
                    done.append(inst)

        # One dictionary lookup per instruction rather than a loop over every
        # routine: with thirty-odd routines the loop was most of the run.
        if addr_map["built"] != (drv["seg"] is not None, sx["dirty"]):
            sx["dirty"] = False
            addr_map["m"] = {}
            for nm in names:
                e = entry_addr(nm)
                if e is not None:
                    addr_map["m"].setdefault(e, []).append(nm)
            addr_map["built"] = (drv["seg"] is not None, sx["dirty"])
        for name in addr_map["m"].get(address, ()):
            k = counts[name]
            counts[name] = k + 1
            if k not in want[name]:
                continue
            want[name].discard(k)
            spec = ROUTINES[name]
            ss = uc.reg_read(UC_X86_REG_SS)
            sp = uc.reg_read(UC_X86_REG_SP)
            cs_now = uc.reg_read(UC_X86_REG_CS)
            nargs = len(spec["args"])
            # A **near** function pushes only IP, so its return address is two
            # bytes and its first argument sits two bytes lower than a far
            # one's. Getting this wrong reads the return address as an
            # argument, which looks like a routine misbehaving.
            near = spec.get("near", False)
            aoff = 2 if near else 4
            stk = uc.mem_read(ss * 16 + sp, aoff + 2 * max(4, nargs))
            inst = {"name": name, "spec": spec, "occ": k, "events": [],
                    "near": near, "aoff": aoff,
                    "ret": ((stk[0] | (stk[1] << 8), cs_now)
                            if near else
                            (stk[0] | (stk[1] << 8), stk[2] | (stk[3] << 8))),
                    "sp": sp, "sp_min": sp, "ax": None, "dx": None,
                    "state": {}, "drv": {}, "regs_out": {},
                    "planes_in": None, "planes_out": None, "state_out": None,
                    "gc_in": None, "mask_in": 0x0F, "src": None,
                    "mem_in": None, "mem_out": None, "dg_base": 0,
                    "alloc_from": 0, "allocs": [],
                    "drv_seg": drv["seg"], "done": False,
                    "opened_at": m.vclock, "abandoned": False}
            if spec.get("regs"):
                inst["args"] = [uc.reg_read(REGS[r]) for r in spec["regs"]]
            else:
                inst["args"] = [stk[aoff + 2 * i] | (stk[aoff + 1 + 2 * i] << 8)
                                for i in range(nargs)]
            if spec.get("src_stack_far"):
                # A far pointer passed on the stack: offset then segment.
                oi, si_, slen = spec["src_stack_far"]
                off = stk[aoff + 2 * oi] | (stk[aoff + 1 + 2 * oi] << 8)
                seg = stk[aoff + 2 * si_] | (stk[aoff + 1 + 2 * si_] << 8)
                inst["src"] = bytes(uc.mem_read(seg * 16 + off, slen))
            if spec.get("src_stack"):
                sseg, aidx, slen = spec["src_stack"]
                off = stk[aoff + 2 * aidx] | (stk[aoff + 1 + 2 * aidx] << 8)
                inst["src"] = bytes(uc.mem_read(
                    uc.reg_read(REGS[sseg]) * 16 + off, slen))
            if spec.get("src_from"):
                sseg, soff, slen = spec["src_from"]
                n2 = (slen if isinstance(slen, int)
                      else uc.reg_read(REGS[slen]) or 0x10000)
                inst["src"] = bytes(uc.mem_read(
                    uc.reg_read(REGS[sseg]) * 16 + uc.reg_read(REGS[soff]),
                    min(n2, 0x10000)))
            dg = base + DGROUP
            # The whole segment, not a declared list of variables. A routine
            # that touches state nobody thought to declare is then caught
            # rather than missed, and near pointers into DGROUP work at all.
            # The whole of conventional memory below the VGA aperture, not
            # just DGROUP: the game holds far pointers into blocks DOS gave it,
            # and a routine that follows one is looking outside DGROUP.
            inst["mem_in"] = bytes(uc.mem_read(0, 0xA0000))
            inst["dg_base"] = dg
            inst["alloc_from"] = len(m.dos_alloc_log)
            if spec.get("planes"):
                inst["planes_in"] = [bytes(p) for p in m.planes]
                inst["gc_in"] = bytes(m.gc[:9])
                inst["mask_in"] = m.map_mask
            open_inst.append(inst)

    def on_sx_ptr(uc, typ, address, size, value, ud):
        # The loader filling in the sound driver's far pointer. Until it does,
        # an sx_overlay routine has no address; watching the write is cheaper
        # than re-reading the pointer on every instruction, and catching it is
        # what makes those routines findable at all - the driver loads after
        # the video one, so keying the rebuild on the video segment misses it.
        sx["dirty"] = True

    m.uc.hook_add(UC_HOOK_CODE, on_code)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc2.UC_X86_INS_OUT)
    m.uc.hook_add(UC_HOOK_MEM_WRITE, on_mem, None, 0xA0000, 0xB0000)
    m.uc.hook_add(UC_HOOK_MEM_READ, on_mem, None, 0xA0000, 0xB0000)
    m.uc.hook_add(UC_HOOK_MEM_WRITE, on_sx_ptr, None,
                  base + 0x26190 + 0x1e7, base + 0x26190 + 0x1eb)

    # An interrupt that fires inside a routine writes hardware of its own, so
    # the machine is not serviced while any instance is open.
    real_timer, real_kbd = m.service_timer, m.service_keyboard
    m.service_timer = lambda: False if open_inst else real_timer()
    m.service_keyboard = lambda: False if open_inst else real_kbd()

    # A watchdog. A routine that waits for an interrupt can never return while
    # the harness is suppressing interrupts, and without this the whole sweep
    # sits inside it - which is how `wait_and_latch_frame` was found, by a run
    # that never finished. Abandoning the instance and saying so is better than
    # hanging, and better than quietly not tracking such routines at all.
    STUCK = 30_000_000

    def on_slice(mm, d):
        for inst in list(open_inst):
            if mm.vclock - inst["opened_at"] > STUCK:
                inst["abandoned"] = True
                open_inst.remove(inst)
                done.append(inst)
                print("  [watchdog] %s occurrence %d ran for %dM instructions "
                      "without returning - abandoned"
                      % (inst["name"], inst["occ"], STUCK // 1_000_000))
        return not any(want.values()) and not open_inst

    drive.drive(m, budget, on_slice=on_slice)
    for inst in done:
        inst["total_seen"] = counts[inst["name"]]
    return done, counts


def sweep():
    """Verify every routine in ONE run of the original, and write the table."""
    lib = load_lib()
    names = [n for n in ROUTINES if not ROUTINES[n].get("unverifiable")]
    skipped_names = [n for n in ROUTINES if ROUTINES[n].get("unverifiable")]
    budget = max(ROUTINES[n].get("budget", 40_000_000) for n in names)
    print("collecting %d routines in one run (budget %dM instructions)..."
          % (len(names), budget // 1_000_000))
    captured, counts = collect_all(names, budget=budget)
    print("captured %d calls\n" % len(captured))

    by_name = {}
    for inst in captured:
        by_name.setdefault(inst["name"], []).append(inst)

    rows = []
    shown = {}
    for name in names:
        spec = ROUTINES[name]
        where = ("VM.OVL VGA:0x%04x" % spec["overlay"]) if spec.get("overlay") \
            else ("SX.OVL SPKR:0x%04x" % spec["sx_overlay"]) \
            if spec.get("sx_overlay") else ("0x%05x" % spec["addr"])
        wanted = spec.get("check_occurrences", [0])
        insts = sorted(by_name.get(name, []), key=lambda i: i["occ"])
        got_occ = [i["occ"] for i in insts]
        results = []
        for inst in insts:
            if inst.get("abandoned"):
                results.append((inst["occ"], False))
                print("--- %s occurrence %d: never returned while interrupts "
                      "were suppressed ---" % (name, inst["occ"]))
                continue
            ok, detail = compare_instance(inst, lib, verbose=False)
            results.append((inst["occ"], ok))
            if not ok and not shown.get(name):
                shown[name] = True
                print("--- %s occurrence %d ---" % (name, inst["occ"]))
                print(detail)
        missing = [o for o in wanted if o not in got_occ]
        ok_all = bool(results) and all(o for _, o in results) and not missing
        if counts[name] == 0:
            # Transcribed, and nothing on these screens calls it. That is not a
            # pass and not a failure: it is an unchecked routine, and saying so
            # is the whole point of separating "transcribed" from "verified".
            rows.append((name, where, None, [], []))
            print("%-24s %-22s TRANSCRIBED, NEVER CALLED on these screens"
                  % (name, where))
            continue
        rows.append((name, where, ok_all, results, missing))
        note = "  (%d calls seen)" % counts[name]
        if missing:
            note = ("  (only %d calls seen; never reached: %s)"
                    % (counts[name], ", ".join(str(o) for o in missing)))
        print("%-24s %-22s %s%s"
              % (name, where, "verified" if ok_all else "NOT VERIFIED", note))

    for n in skipped_names:
        spec = ROUTINES[n]
        where = ("VM.OVL VGA:0x%04x" % spec["overlay"]) if spec.get("overlay") \
            else ("SX.OVL SPKR:0x%04x" % spec["sx_overlay"]) \
            if spec.get("sx_overlay") else ("0x%05x" % spec["addr"])
        rows.append((n, where, None, [], []))
        print("%-24s %-22s TRANSCRIBED, NOT VERIFIABLE  (%s)"
              % (n, where, spec["unverifiable"]))

    lines = ["| routine | address | occurrences checked | result |",
             "| --- | --- | --- | --- |"]
    for name, where, ok, results, missing in rows:
        if ok is None:
            why = ROUTINES[name].get("unverifiable")
            lines.append("| `%s` | %s | - | %s |"
                         % (name, where,
                            ("**transcribed, not verifiable**: " + why) if why
                            else "**transcribed, never called** on these screens"))
            continue
        detail = ", ".join(str(o) for o, _ in results) or "none reached"
        if missing:
            detail += " (missed %s)" % ", ".join(str(o) for o in missing)
        lines.append("| `%s` | %s | %s | %s |"
                     % (name, where, detail, "agreed" if ok else "**not verified**"))
    nver = sum(1 for r in rows if r[2])
    lines.append("")
    lines.append("*%d transcribed, %d verified. Written by "
                 "`tools/verify.py --all`, not by hand - one run of the "
                 "original captures every call.*" % (len(rows), nver))
    table = "\n".join(lines)

    if os.path.exists(STATUS):
        txt = open(STATUS).read()
        if BEGIN in txt and END in txt:
            pre = txt[:txt.index(BEGIN) + len(BEGIN)]
            post = txt[txt.index(END):]
            open(STATUS, "w").write(pre + "\n" + table + "\n" + post)
            print("\nwrote the table into STATUS.md")
    return 0 if all(r[2] is not False for r in rows) else 1


def fmt(e):
    if e is None:
        return "-"
    port, off, val, rd = e
    if port == 0xA000:
        return "A000:%04x %s %02x" % (off, "read " if rd else "write", val)
    return "%s %#05x = %02x" % ("in " if rd else "out", port, val)


if __name__ == "__main__":
    sys.exit(main())
