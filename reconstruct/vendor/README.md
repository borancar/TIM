# Vendored code

Third-party source, compiled without this project's warning flags, because
those are this project's rules and not its authors'.

## ymfm

Aaron Giles' FM synthesis cores, BSD-3-Clause - the licence is in `ymfm/`.
Used for the **OPL2 (YM3812)**, the chip on an AdLib card and on every Sound
Blaster: `SX.OVL`'s `ADL:` driver programs it, and there is nothing in
`TIM.EXE` to transcribe a chip *from*. See `../src/opl.h` for where the line
between the driver and the hardware falls.

`ymfm_opl.cpp` is the one that matters; the ADPCM, PCM and SSG units come with
it through its templates even though a YM3812 contains none of them.

**The same copy the Lemmings reconstruction uses**, deliberately: one OPL2
implementation between the two ports, so a fault found in one is a fault fixed
in both, and `src/opl.h` and `src/opl_ymfm.cpp` are that port's files with
`opl_status` added - which this game needs because its driver *detects* the
card by programming the timers and reading them back, where Lemmings' assumes
the card is there.
