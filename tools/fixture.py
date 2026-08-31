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
# lookup would then never match. See reconstruct/seg0dff.c at 0x11e0b.
PASSWORDS = b"A\r\nBB\r\nCCC\r\n"


def build(out, seed="CATOMATC.TIM"):
    if os.path.abspath(out) == os.path.abspath(GAME):
        raise SystemExit("refusing to build the fixture over the game folder")

    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(GAME, out)

    sub = os.path.join(out, "SUBDIR")
    os.makedirs(sub, exist_ok=True)
    shutil.copy(os.path.join(GAME, seed), sub)

    with open(os.path.join(out, "password.txt"), "wb") as f:
        f.write(PASSWORDS)

    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, help="where to build it")
    ap.add_argument("--seed", default="CATOMATC.TIM",
                    help="a machine to put in the subdirectory, so the picker "
                         "has something to list there")
    args = ap.parse_args()

    out = build(args.out, args.seed)
    print("%s: %d entries, SUBDIR and password.txt added"
          % (out, len(os.listdir(out))))
    print()
    print("reaches the navigation and the codes with, for example:")
    print("  uv run python tools/verify.py --all --game-dir %s \\" % out)
    print("      --from <snap> --budget 40000000 \\")
    print("      --click 10:170:152 --click 200:100:128 --click 400:100:128 \\")
    print("      --only path_join,path_is_root,path_up")
    return 0


if __name__ == "__main__":
    sys.exit(main())
