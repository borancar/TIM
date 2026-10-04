/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **`atan2_long`, and nothing else.** This file corresponds to the original's
 * **code segment 2d29**, image 0x2d296..0x2d3b7 - one routine, a module of its
 * own. It was part of `trig.c` until 2026-09-26, which is segment 2a04, and it
 * is not that module's: it was not even built by that compiler.
 *
 * JUDGE: compiler 1.01
 * JUDGE: built-with -mm
 *
 * **Turbo C++ 1.0x built it**, not the 3.0 that built the game: it reserves its
 * two flag bytes with `dec sp / dec sp`, which 3.0 does not write, and only
 * TC++ 1.0x stores a negated `long` low word first as the image does - Borland
 * C++ 2.0 matches everything else, but stores the high word first however the
 * negation is spelled (`-a`, `0 - a`, `a *= -1` and the cast were all tried). Compiled here by TC++ 1.01 `-mm` it is the image's
 * 289 bytes, far calls and all. So it came into the link already compiled -
 * most likely a library of Dynamix's - and it has no `-O`: the jump over the
 * epilogue's `jmp` is still there.
 *
 * Written so that both compilers take it: the casts that keep the host free of
 * undefined behaviour (a negated `long`, a shift into the sign bit) are free
 * under TCC, which makes the same instructions with or without them.
 */
#include "tim.h"

/*
 * 0x2ede0
 *
 * Arctangent of two 32-bit values - `atan2` - answering an angle in the
 * whole-turn-is-0x10000 space.
 *
 * Both arguments are folded to their magnitudes first, each remembering
 * whether it was negative, and the octant is put back at the end: a negative
 * first argument reflects the result about 0x800, a negative second about
 * 0x1000.
 *
 * Within the octant only a ratio below one is ever looked up, because
 * `g_arctan_table` has no entry for one. Whichever magnitude is smaller is
 * shifted left by 9 and divided by the larger, giving 0..511; when the first is
 * the smaller the answer is complemented within the octant, `0x400 - x`.
 *
 * Equal magnitudes are handled separately rather than by division, which is
 * what keeps index 512 out of reach: equal and non-zero is 0x200, and both zero
 * is 0x400. The code then tests whether the *second* is zero and would answer
 * 0 - that is dead, since reaching it means the two were equal and the first
 * was already known non-zero.
 *
 * Finally a quarter turn is subtracted and the result scaled up by 16, which is
 * what moves it from the table's 0x1000-per-turn units into the 0x10000 space
 * the sine tables use.
 *
 * **The spelling is the compiler's evidence.** The flags are set inside the
 * test - `or al,al` straight after the byte is stored is the value still in
 * AL - and are bytes, two of them in one word below BP. The division is the
 * runtime's *signed* `F_LDIV@`. `r = r - 0x400` goes through AX where
 * `r -= 0x400` subtracts from SI in place.
 */
int16_t atan2_long(int32_t a, int32_t b)
{
    uint8_t neg_a, neg_b;
    int16_t r;

    if ((neg_a = a < 0) != 0)
        a = (int32_t)-(uint32_t)a;
    if ((neg_b = b < 0) != 0)
        b = (int32_t)-(uint32_t)b;
    if (a < b)
        r = 0x400 - arctan_lookup((uint16_t)((int32_t)((uint32_t)a << 9) / b));
    else if (a > b)
        r = arctan_lookup((uint16_t)((int32_t)((uint32_t)b << 9) / a));
    else if (a == 0)
        r = 0x400;
    else if (b == 0)
        r = 0;
    else
        r = 0x200;
    if (neg_a)
        r = 0x800 - r;
    if (neg_b)
        r = 0x1000 - r;
    r = r - 0x400;
    return (int16_t)((uint16_t)r << 4);
}
