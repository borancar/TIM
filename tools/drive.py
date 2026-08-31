"""Run the machine, servicing it the way the emulator's own event loop does.

Every tool here needs the same loop - run a slice, deliver the timer and the
keyboard, repeat - and writing it out again in each one is how they drift apart.
It also advances the virtual clock, which is what makes a run reproducible:
with `ips` set, the guest's sense of time comes from how many instructions it
has executed rather than from the host's wall clock, so the same call reaches
the same state every time.

This file is the port's own tooling; it is not a transcription.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP

# Instructions per emulated second. A guess, and recorded as one: the game
# was written for a 286-class machine and this has not been measured against
# the original's own cycle counts yet. It sets the frame rate the guest
# believes it is achieving, so it must be settled before any timing claim.
DEFAULT_IPS = 2_000_000

# Instructions per slice. This is not a performance knob to be turned up: the
# emulator delivers at most one timer tick per service call and deliberately
# never catches up, so a slice longer than one PIT period silently drops
# ticks and the guest spins waiting for them. Measured: at ips 2,000,000 a
# step of 2000 reaches page flip 40 in 28.3M instructions, and a step of
# 20000 fails to reach it in 269M.
DEFAULT_STEP = 2000


def machine(ips=DEFAULT_IPS, verbose=False, snapshot=None, **kw):
    """The machine every tool here runs on.

    `snapshot` starts it from a state saved by `tools/snapshot.py` rather than
    from the program's entry point. The machine is built the ordinary way first
    - hooks and all, because those are not state - and the saved state is put
    into it afterwards, so a snapshot taken under one tool replays under any
    other.
    """
    tim.game_dir()
    m = tim.TimMachine(tim.UNPACKED_EXE, **kw)
    m.verbose = verbose
    m.max_insns = 10 ** 12
    m.vclock_ips = ips
    if snapshot:
        import snapshot as _snap

        _snap.restore(m, snapshot)
    return m


def drive(m, instructions, step=DEFAULT_STEP, on_slice=None):
    """Run `instructions` emulated instructions in slices, servicing between.

    A small step is not a cost here - it is the point. The guest waits on the
    timer, and the sooner a tick arrives after it starts waiting the less it
    spins.
    """
    addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
    done = 0
    while done < instructions:
        try:
            m.uc.emu_start(addr, 0, count=step)
        except UcError as e:
            return "cpu error: %s" % e
        if m.finished:
            return "exited: %s" % m.finished
        done += step
        m.vclock += step
        addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
        m.service_keyboard()
        m.service_timer()
        addr = m._reg(UC_X86_REG_CS) * 16 + m._reg(UC_X86_REG_IP)
        if on_slice is not None and on_slice(m, done):
            return "stopped by caller after %d instructions" % done
    return None
