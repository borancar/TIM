"""The one local door to the shared emulator.

Every tool in this repository reaches `dos_emulator` through this module rather
than importing it directly, so that when the shared code moves underneath there
is one file to fix instead of nine. It carries the things that are specific to
The Incredible Machine and nothing else: where the game's files are, which
executable is the recovered one, and the segment its image is loaded at.

This file is the port's own; it is not a transcription of anything.
"""
import os
import sys

import dos_emulator
from dos_emulator import DosMachine, VgaDos, set_game_dir

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME_DIR = os.path.join(REPO, "incredible-machine")

PACKED_EXE = os.path.join(GAME_DIR, "TIM.EXE")
UNPACKED_EXE = os.path.join(REPO, "out", "TIM.unpacked.exe")

# TIM.EXE is packed with LZEXE 0.91 (the "LZ91" tag at offset 0x1c of the
# header). tools/unlzexe.py recovers it by running the stub; see
# docs/executable.md.
PACKED_STUB_CS = 0x1AA0        # from the packed header's CS field
PACKED_STUB_IP = 0x000E

# Addresses in this project are *image offsets* - the byte offset into the
# recovered image, which is what a disassembly lists. The original entry point
# is 0000:0000, so an image offset and a `seg:off` with seg 0 coincide.
IMAGE_BASE_SEG = 0x0000


def game_dir():
    set_game_dir(GAME_DIR)
    return GAME_DIR


# ---------------------------------------------------------------- the machine
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                               UC_X86_REG_DX, UC_X86_REG_DS,
                               UC_X86_REG_ES, UC_X86_REG_EFLAGS)

# INT 10h AH=1Ah display combination codes. 0x08 is "VGA with a colour
# analogue monitor", which is what the machine this project emulates is.
DCC_VGA_COLOUR = 0x08


class TimMachine(VgaDos):
    """The emulated PC as The Incredible Machine expects to find it.

    Everything here is a *gap in the shared emulator* that this game happens to
    be the first to hit, not something specific to the game. Both additions are
    plain VGA BIOS services, so they belong upstream in `dos_emulator`; they
    live here until they are pushed, and STATUS.md records that.

    Without them the game's adapter probe at image 0x225d2 reads a BX the BIOS
    never wrote, concludes there is no VGA and no EGA, fails to load its video
    driver overlay VM.OVL, prints "Unable to initialize vm." and exits.
    """

    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        # The recovered header asks for every paragraph it can get
        # (maxalloc 0xFFFF), and that is what real DOS gives an EXE: the whole
        # of free conventional memory, as one block belonging to the program.
        # Nothing is free until the C runtime hands the tail back with
        # AH=4Ah. Starting a free arena just above image+minalloc instead -
        # which is what the shared emulator does - puts DOS's blocks *inside*
        # the program's own DGROUP, and Borland's large-model startup places
        # the stack at the top of a 64 KB DGROUP, so the first allocation
        # quietly overwrites the stack. It showed up as a `retf` into zeroed
        # memory at 8a2e:5752 after a million instructions, which looks like
        # anything but an allocator bug.
        self.prog_paras = self.mem_top - self.psp_seg
        self.arena = []
        # --------------------------------------------------------- the clock
        # Upstream paces the timer IRQ and the retrace bit on the *host* wall
        # clock, so a guest that waits for a tick spins for however many
        # instructions the host can execute in the meantime. This game does
        # exactly that: its INT 08h handler sets word_5754 and the main loop
        # at image 0x0aaca spins on it, which measured at **64% of all basic
        # block executions** in a start-up run - 31.7 million iterations of a
        # four-instruction wait.
        #
        # Driving the clock from an emulated instruction count instead makes
        # the guest see a steady machine, cuts the spin to nothing, and - the
        # part that matters more - makes a run *deterministic*, so a capture
        # taken at a given point is the same capture on another machine and
        # after a rebuild. `vclock_ips` of 0 keeps upstream's behaviour.
        self.vclock = 0
        self.vclock_ips = 0
        self.dos_alloc_log = []      # (paragraphs asked, seg, largest, failed)
        # handle -> [name, position]. What the port needs to open the same
        # files at the same offsets; see _dos below.
        self.dos_files = {}

    def _elapsed(self):
        if self.vclock_ips:
            return self.vclock / self.vclock_ips
        return super()._elapsed()

    def save_snapshot(self, name):
        """Write the whole machine out, for `shift+F2` in a windowed run.

        The emulator's window loop offers the key and asks the machine whether
        it can do anything with it, the same way `stop_requested` is offered -
        so what a snapshot *is* stays here, where the game is known, and the
        window stays generic.

        The point of the key is the states no tool can reach on its own: the
        menus, the level editor and the game proper are behind someone pressing
        something, and they are why coverage stops where it does. Play to one,
        press shift+F2, and every tool can start there afterwards with
        `drive.machine(snapshot=...)`.

        `.snap` is appended if the caller did not, and the file lands in the
        working directory - the emulator names captures the same way.

        **An existing file is never overwritten.** The name comes from a counter
        the window loop restarts at 1 every run, so a second session writes
        `snap01` over the first session's `snap01` - which is exactly what
        happened, and cost a captured state that had taken minutes of play to
        reach. A snapshot is expensive to make and cheap to keep, so a clash
        walks the number forward instead of destroying what is there.
        """
        here = os.path.dirname(os.path.abspath(__file__))
        if here not in sys.path:
            sys.path.insert(0, here)
        import snapshot as _snap

        stem = name[:-5] if name.endswith(".snap") else name
        path = stem + ".snap"
        if os.path.exists(path):
            base = stem.rstrip("0123456789") or stem
            n = 1
            while os.path.exists("%s%02d.snap" % (base, n)):
                n += 1
            path = "%s%02d.snap" % (base, n)

        _snap.save(self, path)
        return os.path.abspath(path)

    def _cstring(self, addr, limit=128):
        out = bytearray()
        for i in range(limit):
            b = self.uc.mem_read(addr + i, 1)[0]
            if b == 0:
                break
            out.append(b)
        return out.decode("latin-1")

    def _dos(self):
        ax = self._reg(UC_X86_REG_AX)
        ah = ax >> 8
        if ah in (0x3D, 0x3E, 0x3F, 0x42):
            # Track open files, so tools/verify.py can prime the port with the
            # same handles at the same offsets. Without this a routine that
            # reads a file is unverifiable however faithfully it is
            # transcribed: the harness seeds guest memory, and a file's handle
            # and position are not in guest memory. Same reason AH=48h is
            # logged above, and the same remedy.
            if ah == 0x3D:
                seg = self.uc.reg_read(UC_X86_REG_DS) & 0xFFFF
                off = self._reg(UC_X86_REG_DX) & 0xFFFF
                name = self._cstring(seg * 16 + off)
                r = super()._dos()
                if not (self.uc.reg_read(UC_X86_REG_EFLAGS) & 1):
                    self.dos_files[self._reg(UC_X86_REG_AX) & 0xFFFF] = \
                        [name, 0]
                return r
            handle = self._reg(UC_X86_REG_BX) & 0xFFFF
            want = self._reg(UC_X86_REG_CX) & 0xFFFF
            r = super()._dos()
            failed = bool(self.uc.reg_read(UC_X86_REG_EFLAGS) & 1)
            entry = self.dos_files.get(handle)
            if ah == 0x3E:
                self.dos_files.pop(handle, None)
            elif entry is not None and not failed:
                if ah == 0x3F:
                    entry[1] += self._reg(UC_X86_REG_AX) & 0xFFFF
                else:
                    entry[1] = ((self._reg(UC_X86_REG_DX) & 0xFFFF) << 16) \
                        | (self._reg(UC_X86_REG_AX) & 0xFFFF)
            return r
        if ah == 0x48:
            # Record what DOS answered, so tools/verify.py can prime the port
            # with it. The port has no DOS and no arena; without this, every
            # routine that allocates would be unverifiable, and there are
            # twenty-two callers of just one of them.
            want = self._reg(UC_X86_REG_BX) & 0xFFFF
            r = super()._dos()
            got_ax = self._reg(UC_X86_REG_AX) & 0xFFFF
            got_bx = self._reg(UC_X86_REG_BX) & 0xFFFF
            cf = bool(self.uc.reg_read(UC_X86_REG_EFLAGS) & 1)
            self.dos_alloc_log.append((want, got_ax, got_bx, cf))
            return r
        if ah == 0x4A and (self.uc.reg_read(UC_X86_REG_ES) & 0xFFFF) == self.psp_seg:
            # Resize the program's own block. DOS shrinks it in place and the
            # tail becomes the free arena; growing beyond memory fails with
            # the largest size available, as DOS does.
            want = self._reg(UC_X86_REG_BX) & 0xFFFF
            avail = self.mem_top - self.psp_seg
            self.dos_counts[ah] += 1
            if want > avail:
                self._cf(True)
                self._set(UC_X86_REG_AX, 8)
                self._set(UC_X86_REG_BX, avail)
                return
            self.prog_paras = want
            top = self.psp_seg + want
            keep = [b for b in self.arena if b[0] >= top and b[2]]
            free_top = max([b[0] + b[1] for b in keep], default=top)
            self.arena = keep + ([[free_top, self.mem_top - free_top, False]]
                                 if self.mem_top > free_top else [])
            self._mem_coalesce()
            self._fop(f"RESIZE program block to {want:#x} paragraphs; "
                      f"free memory now starts at {top:04x}")
            self._cf(False)
            return
        return super()._dos()

    # ------------------------------------------------------------ the CRTC
    # The VGA BIOS's own CRTC table for mode 12h, indices 0x00..0x18. Without
    # it the register file starts empty, reads come back 0, and the game's
    # read-modify-write at image 0x8f77 - which reads Overflow and Maximum
    # Scan Line back before setting one bit in each - silently clears every
    # timing bit the BIOS had put there. It happens to land on the same
    # blanking line either way, which is exactly what makes the gap dangerous:
    # it is invisible until something else reads a register back.
    CRTC_MODE12 = (0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00,
                   0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C,
                   0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3, 0xFF)

    def _seed_crtc(self):
        for i, v in enumerate(self.CRTC_MODE12):
            self.crtc[i] = v
        self.start_addr = 0
        self.crtc_offset = self.crtc[0x13]
        self._update_addr_mode()

    def service_keyboard(self):
        """Deliver keys, and honour the window being closed *immediately*.

        Closing the window sets `running = False` in the shared emulator's
        event loop - but that loop only runs between instruction chunks, and
        it then prints the whole census before returning, so the window shuts
        and the game appears to carry on. Closing a window should kill the
        program at once, which is not the same thing as pressing Esc, and Esc
        is what The Incredible Machine puts its control panel on.

        This is polled between slices, which is as fine-grained as the machine
        gets, and it exits the process rather than unwinding: nothing is
        written to disk - the host filesystem is opened read-only - so there
        is nothing to lose by not unwinding.
        """
        if self._display_up():
            import pygame
            for ev in pygame.event.get(pygame.QUIT):
                print("  [ctl] window closed - quitting")
                sys.stdout.flush()
                os._exit(0)
        return super().service_keyboard()

    @staticmethod
    def _display_up():
        try:
            import pygame
            return bool(pygame.display.get_init()
                        and pygame.display.get_surface())
        except Exception:
            return False

    def _on_in(self, uc, port, size, user):
        if port == 0x3C7:
            # The DAC state register. Bits 0-1 read 3 after the *write* index
            # (0x3C8) was last set and 0 after the read index (0x3C7), which
            # is how a program asks whether the DAC is mid-triple. Upstream
            # answers 0 unconditionally, so the game's palette routine at
            # VGA:0x0ec1 always writes a resynchronising byte that real
            # hardware would not have asked for. It is harmless - the very
            # next write to 0x3C8 discards it - but a divergence that happens
            # not to matter is still a divergence, and this one is two lines.
            self.port_in[port] += 1
            return 0x03 if getattr(self, "dac_write_mode", True) else 0x00
        if port == 0x3D5:
            self.port_in[port] += 1
            return self.crtc.get(self.crtc_index, 0)
        return super()._on_in(uc, port, size, user)

    # ------------------------------------------------------- vertical blank
    def start_vertical_blank(self):
        """The scan line the CRTC starts blanking at, or None if untouched.

        Ten bits spread across three registers, as the VGA does it: the low
        eight in Start Vertical Blank (0x15), bit 8 in Overflow (0x07) bit 3,
        bit 9 in Maximum Scan Line (0x09) bit 5.
        """
        if 0x15 not in self.crtc:
            return None
        return (self.crtc[0x15]
                | ((self.crtc.get(0x07, 0) >> 3) & 1) << 8
                | ((self.crtc.get(0x09, 0) >> 5) & 1) << 9)

    def framebuffer(self):
        """The displayed picture, with blanked scan lines actually blank.

        The game leaves Vertical Display End at the BIOS's 479 and instead
        moves *blanking* up, to line 399 for its 640x400 screens (and 470 for
        the Sierra logo). Real hardware still scans 480 lines but draws
        nothing past the blanking line.

        **The blanking line itself is displayed**, so the picture is `svb + 1`
        rows: 400 and 471. Two things say so. DOSBox, an independent model of
        the same hardware, reports this game as "VGA 640x471 16-colour graphics
        mode" for the Sierra logo and "VGA 640x400" for the game. And the
        game's own memory holds a full 640-pixel row at y=399 - it draws that
        row, which a program has no reason to do for a line it cannot show.
        This was 400 and 471 nowhere and 399 and 470 everywhere until the
        DOSBox reading turned up; the port and this file share the convention,
        so agreeing with each other proved nothing about it.

        Ignoring that is not cosmetic. The game page-flips by writing only the
        high byte of the start address, between 0x0000 and 0x8200, and two
        640x400 pages fit in a 64 KB plane only because the tail is never
        shown. Rendering the full 480 lines wraps page 1 around the plane and
        paints the top of the *other* page across the bottom eighty rows of
        the screen - which looks like a blitter bug in the game and is
        entirely an artefact of the reference.

        Generic VGA behaviour, so this belongs upstream; STATUS.md records it.
        """
        fb = super().framebuffer()
        svb = self.start_vertical_blank()
        if svb is None:
            return fb
        shown = svb + 1
        if shown >= self.height:
            return fb
        w = self.width
        out = bytearray(fb)
        del out[shown * w:]
        out.extend(b"\x00" * ((self.height - shown) * w))
        return bytes(out)

    def _on_out(self, uc, port, size, value, user):
        if port == 0x3C8:
            self.dac_write_mode = True
        elif port == 0x3C7:
            self.dac_write_mode = False
        return super()._on_out(uc, port, size, value, user)

    def _bios_video(self):
        ax = self._reg(UC_X86_REG_AX)
        ah, al = ax >> 8, ax & 0xFF
        bx = self._reg(UC_X86_REG_BX)

        if ah == 0x00 and (al & 0x7F) == 0x12:
            r = super()._bios_video()
            self._seed_crtc()
            return r

        if ah == 0x1A and al == 0x00:
            # Get display combination code. AL=0x1A acknowledges that the
            # call exists at all - a pre-VGA BIOS leaves AL alone, which is
            # how a program tells the difference. BL is the active display,
            # BH the alternate (none).
            self.int10_fn[ah] += 1
            self._set(UC_X86_REG_AX, (ax & 0xFF00) | 0x1A)
            self._set(UC_X86_REG_BX, DCC_VGA_COLOUR)
            return

        if ah == 0x12 and (bx & 0xFF) == 0x10:
            # Get EGA information. A VGA BIOS answers this too. BH=0 colour,
            # BL=3 for 256K of display memory, CH feature bits, CL switches.
            # The caller's test is that BX changed from the 0x10 it passed in.
            self.int10_fn[ah] += 1
            self._set(UC_X86_REG_BX, 0x0003)
            self._set(UC_X86_REG_CX, 0x0009)
            return

        return super()._bios_video()


def make_machine(args):
    """Build the machine for a run driven by the shared emulator's main()."""
    game_dir()
    return TimMachine(args.program, blaster=args.blaster,
                      psp_seg=args.psp_seg, cmdline=args.cmdline)
