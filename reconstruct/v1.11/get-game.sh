#!/bin/sh
# Create game/, the game's own files, from GOG's installer of The Even More
# Incredible Machine. Ours, not a transcription; GPL-2.0 like the rest of this
# repository's tooling.
#
#     ./get-game.sh [setup_the_even_more_incredible_machine_2.1.0.24.exe] [game]
#
# The installer is GOG's (gog.com, "The Even More Incredible Machine"), and only
# the build whose SHA-256 is below is accepted: it holds TIM.EXE 1.11, the
# version this tree reconstructs byte for byte. It is not distributed here -
# buy it and put it beside this script, or name it.
#
# innoextract unpacks it. Only the installer's `app/` - the game's own files -
# is taken, and of that, not DOSBox (`app/DOSBOX/`, `app/dosbox*.conf`) - this
# tree is what runs the game - nor GOG's empty `__support/`.
set -eu

SHA256=2746504f970f0490b94f6472fddf9cf937770a85a1ed0f9e973a45c282ffcc67
HERE=$(cd "$(dirname "$0")" && pwd)
SETUP=${1:-$HERE/setup_the_even_more_incredible_machine_2.1.0.24.exe}
GAME=${2:-$HERE/game}

if [ ! -f "$SETUP" ]; then
    echo "no installer at $SETUP - GOG's setup_the_even_more_incredible_machine_2.1.0.24.exe" >&2
    exit 1
fi
command -v innoextract >/dev/null || { echo "innoextract is not installed" >&2; exit 1; }

sum=$(sha256sum "$SETUP" | cut -d' ' -f1)
if [ "$sum" != "$SHA256" ]; then
    echo "$SETUP: SHA-256 $sum, not GOG's 2.1.0.24 ($SHA256)" >&2
    exit 1
fi

if [ -e "$GAME" ]; then
    echo "$GAME exists - remove it first to extract it afresh" >&2
    exit 1
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
innoextract --silent --include app --output-dir "$TMP" "$SETUP"

mkdir -p "$GAME"
for f in "$TMP"/app/* "$TMP"/app/.[!.]*; do
    [ -e "$f" ] || continue
    case "$(basename "$f")" in
        DOSBOX|dosbox*.conf|__support) ;;
        *) mv "$f" "$GAME"/ ;;
    esac
done

if [ ! -f "$GAME/TIM.EXE" ]; then
    echo "no TIM.EXE among what was extracted" >&2
    exit 1
fi
echo "$GAME: $(ls "$GAME" | wc -l) files, TIM.EXE among them"
