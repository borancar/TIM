"""Run The Incredible Machine under the emulator, with this game's machine.

Every flag the shared emulator understands works here; see `--help`. This is
the reference the reconstruction is checked against.

This file is the port's own tooling; it is not a transcription.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
import dos_emulator


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        argv = [tim.UNPACKED_EXE] + argv
    if not any(a == "--game-dir" or a.startswith("--game-dir=") for a in argv):
        argv += ["--game-dir", tim.GAME_DIR]
    return dos_emulator.main(argv, make_machine=tim.make_machine)


if __name__ == "__main__":
    sys.exit(main())
