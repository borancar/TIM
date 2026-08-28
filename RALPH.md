# The loop's task

**Goal.** Transcribe routines from the disassembly until the two intro screens
after the Sierra logo — the **title screen** (page flips 6..279) and the
**credits screen** (from flip 280), both of which animate and so exercise real
game logic — are reproduced by the C port in `reconstruct/` with **0 differing
pixels** against captures taken from the original under the emulator.

`CLAUDE.md` is authoritative on how to work; this file only says what "done"
means and how to settle it.

## Each pass

1. **Re-derive the remaining list.** Run `tools/codemap.py` and the coverage
   count fresh every pass. Never work from a list remembered from an earlier
   pass: it goes stale the moment a newly transcribed routine reveals a call to
   something unmapped.
2. Take the next untranscribed routine that the two screens reach. Disassemble
   it with `tools/disasm.py`, read it, transcribe it, wire it into the
   verifier.
3. Verify it **against the original**, not against the screen. If no reachable
   state calls it, say so in `STATUS.md` rather than counting it as done.
4. Update `STATUS.md` with **measured** numbers, and record retractions when a
   claim turns out to be wrong.

## Settling "done"

    uv run python tools/capture.py --flip 10 --flip 300 --out out/ref
    # build the port, render the same two frames, then:
    uv run python tools/diff_png.py --capture out/ref/flip0010.scrn \
        --raw out/port10.raw --name out/title
    uv run python tools/diff_png.py --capture out/ref/flip0300.scrn \
        --raw out/port300.raw --name out/credits

Done is **`differing : 0`** on both, from a capture regenerated in the same
run - never a stale one.

Frame-by-frame is available and is the stronger check: `tools/capture.py`
captures on the guest's own page-flip cue with a deterministic virtual clock,
so a whole run of consecutive flips can be compared, not just two.

## Traps that apply here specifically

- **A blank or mid-fade capture scores as a perfect match.** Judge a capture on
  its rendered colours, not its indices.
- **When a comparison is not zero, write the three images and look at them**
  before proposing any rule about pixels. A model that fits at 96% is wrong,
  and the shape of the wrongness usually means a second thing is being drawn.
- **When a difference survives every hypothesis about the port**, start
  doubting the reference. Four emulator gaps have already been found this way.
- **Count the parameters.** If the port has more free knobs than the original
  has variables, a perfect score is telling you about the knobs.
- The screens **animate**, so a frame is only meaningful next to the flip
  number it came from.

## Do not stop to ask permission to continue

A long task is a reason to continue, not to check in. Involve the user only
for: behaviour the emulator genuinely cannot settle, a scope decision about
whether a screen should be ported at all, anything outward-facing or hard to
reverse, or the discoverable set being exhausted.
