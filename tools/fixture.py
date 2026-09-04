"""Build a game directory with the things the real one happens not to have.

Several routines are unreachable against `incredible-machine/` for reasons that
have nothing to do with the port: the folder has no subdirectory, so the
picker's navigation is never called, and it has `CODES.TXT` rather than
`PASSWORD.TXT`, so the password lookup always fails its open and the score-code
arithmetic behind it never runs.

That is a property of the *files*, not of the game, and it is fixed by copying
the folder and adding to the copy. Nothing here writes to the game's own
directory, and `tools/verify.py --game-dir` points both sides at the result -
the emulator through `set_game_dir` and the port through `io_set_game_dir` - so
the comparison is still between the two of them and not between two
filesystems.

What each addition reaches:

    SUBDIR/         path_join, path_is_root, path_up, and the "." / ".."
                    comparisons in the listing fill. `path_join` is the one
                    with the deliberate off-by-one at both ends, stripping the
                    angle brackets `sub_13a8a` writes round a directory.

    password.txt    password_to_level finding a line, and with it
                    score_code_to_score, parse_base, string_reverse and
                    game_fread_line. Without the file the lookup answers -1
                    before reading anything and none of the rest runs.

    RESOURCE.CFG    --sound-device rewrites the byte that chooses the driver,
                    so a routine in a chunk this installation does not ask for
                    can be reached at all. The shipped file says 0 - the PC
                    speaker - so `GMD:`, `ADL:` and the rest are dead code
                    against the real folder, and `sx_seg` in tools/verify.py
                    reads whichever one the loader put there. Device 7 is
                    General MIDI; docs/sound-driver.md has the table.

The passwords are one, two and three letters so that a code can be typed with
`--key` in a handful of presses; what they are does not matter, only that the
first line matches what is typed.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.path.join(ROOT, "incredible-machine")

# CRLF, because `game_fread_line` puts its terminator at `[si - 1]` - over the
# byte *before* the newline - which removes the CR and the LF in one store. A
# file with Unix endings would lose the last character of every line, and the
# lookup would then never match. See reconstruct/game.c at 0x11e0b.
PASSWORDS = b"A\r\nBB\r\nCCC\r\n"


def build(out, seed="CATOMATC.TIM", sound_device=None,
          sound_module=None):
    if os.path.abspath(out) == os.path.abspath(GAME):
        raise SystemExit("refusing to build the fixture over the game folder")

    # Said rather than raised. A traceback out of `copytree` names a path and
    # not the reason, and the reason - the game's files are not here - is the
    # only thing the reader can act on.
    if not os.path.isdir(GAME):
        raise SystemExit("no game directory at %s - the fixture is a copy of "
                         "it, so there is nothing to copy" % GAME)
    if not os.path.isfile(os.path.join(GAME, seed)):
        raise SystemExit("no %s in %s - --seed names a machine to put in the "
                         "subdirectory, so the picker has something to list "
                         "there" % (seed, GAME))

    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(GAME, out)

    sub = os.path.join(out, "SUBDIR")
    os.makedirs(sub, exist_ok=True)
    shutil.copy(os.path.join(GAME, seed), sub)

    with open(os.path.join(out, "password.txt"), "wb") as f:
        f.write(PASSWORDS)

    if sound_device is not None or sound_module is not None:
        cfg = os.path.join(out, "RESOURCE.CFG")
        with open(cfg, "rb") as f:
            b = bytearray(f.read())

        # Three bytes: something the game stores and never reads, the sound
        # device, and the sound module. A short file is padded with the
        # defaults game_startup uses when there is no file at all - device 0
        # and module -2 - so a truncated one cannot turn into a wrong index.
        while len(b) < 3:
            b.append(0 if len(b) < 2 else 0xfe)

        if sound_device is not None:
            b[1] = sound_device & 0xff
        if sound_module is not None:
            b[2] = sound_module & 0xff

        with open(cfg, "wb") as f:
            f.write(bytes(b))

    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, help="where to build it")
    ap.add_argument("--seed", default="CATOMATC.TIM",
                    help="a machine to put in the subdirectory, so the picker "
                         "has something to list there")
    ap.add_argument("--sound-device", type=int, default=None,
                    help="rewrite RESOURCE.CFG's device byte, so a driver this "
                         "installation does not ask for is the one loaded. "
                         "7 is General MIDI; see docs/sound-driver.md")
    ap.add_argument("--sound-module", type=int, default=None,
                    help="rewrite RESOURCE.CFG's module byte the same way. "
                         "0 is ASB:, the Sound Blaster's digitised half; "
                         "0xfe means load none")
    args = ap.parse_args()

    out = build(args.out, args.seed, args.sound_device, args.sound_module)
    print("%s: %d entries, SUBDIR and password.txt added"
          % (out, len(os.listdir(out))))
    if args.sound_device is not None or args.sound_module is not None:
        with open(os.path.join(out, "RESOURCE.CFG"), "rb") as f:
            print("RESOURCE.CFG: %s" % f.read().hex(" "))
    print()
    print("reaches the navigation and the codes with, for example:")
    print("  uv run python tools/verify.py --all --game-dir %s \\" % out)
    print("      --from <snap> --budget 40000000 \\")
    print("      --click 10:170:152 --click 200:100:128 --click 400:100:128 \\")
    print("      --only path_join,path_is_root,path_up")
    return 0


if __name__ == "__main__":
    sys.exit(main())
