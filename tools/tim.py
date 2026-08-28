"""The one local door to the shared emulator.

Every tool in this repository reaches `dos_emulator` through this module rather
than importing it directly, so that when the shared code moves underneath there
is one file to fix instead of nine. It carries the things that are specific to
The Incredible Machine and nothing else: where the game's files are, which
executable is the recovered one, and the segment its image is loaded at.

This file is the port's own; it is not a transcription of anything.
"""
import os

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
                               UC_X86_REG_ES)

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

    def _dos(self):
        ax = self._reg(UC_X86_REG_AX)
        ah = ax >> 8
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
        nothing from the blanking line down.

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
        if svb is None or svb >= self.height:
            return fb
        w = self.width
        out = bytearray(fb)
        del out[svb * w:]
        out.extend(b"\x00" * ((self.height - svb) * w))
        return bytes(out)

    def _bios_video(self):
        ax = self._reg(UC_X86_REG_AX)
        ah, al = ax >> 8, ax & 0xFF
        bx = self._reg(UC_X86_REG_BX)

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
