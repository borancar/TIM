/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Scores as codes**: turning a score into the password-style code the game
 * shows, and back.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x02809..0x02ac0,
 * split out of machine.c on 2026-09-27. **Both ends of this file are ours.**
 * Nothing proves a module boundary on either side: these routines have no
 * data, and no call reaches back across either end, so they may be the tail
 * of goals.c's module or a module of their own. Neither reading changes a
 * byte of the image - `_TEXT` is byte-aligned, a module with no data leaves
 * no trace in DGROUP, and only a backward call records which file a routine
 * was in - so the file follows the subject.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT -O -Z
 */
#include <string.h>
#ifdef __TURBOC__
#include <stdlib.h>
#else
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x033ed
 *
 * **A score into a score code**, the exact inverse of `score_code_to_score`
 * below, and worth reading beside it - every asymmetry here has a matching
 * step there.
 *
 * `text` arrives holding the password already, and the code is appended to it,
 * so this both reads its first three characters and extends it.
 *
 * The five hex digits are produced by a trick rather than by padding: the
 * score has 0x100000 added before `ltoa`, which guarantees a
 * sixth digit, and the leading `1` is then overwritten with the dash. So the
 * separator and the fixed width are the same operation, and a score of zero
 * comes out `-00000` rather than `-0`.
 *
 * The checksum is the score multiplied by each of the password's first three
 * characters and summed - not the sum of the characters times the score, which
 * is the same number, but the original does three multiplies and this
 * transcribes them.
 *
 * `0` becomes `Z` and both `O` and lowercase `o` become `Y`, so nothing a
 * player has to copy off the screen can be confused for something else. The
 * decoder undoes exactly the first two; the lowercase `o` has no counterpart
 * there because by then the text has been uppercased.
 *
 * The result is uppercased in place at the end.
 */
void score_to_code(int32_t score, register char *text)
{
    char five[8];                       /* [bp-8], the score digits */
    int32_t sum;                        /* [bp-0xc] */
    char sumt[20];                      /* [bp-0x20], the checksum text */
    char code[40];                      /* [bp-0x48], the answer */
    register char *si;

    sum = score + 0x100000L;
    ltoa(sum, five, 0x10);
    five[0] = '-';                      /* over the digit the add forced */
    code[0] = 0;
    strcat(code, five);

    sum = LONG_MUL(score, (uint8_t)text[0]);
    sum += LONG_MUL(score, (uint8_t)text[1]);
    sum += LONG_MUL(score, (uint8_t)text[2]);
    ltoa(sum, sumt, 0x22);
    strcat(code, sumt);

    for (si = code; *si != 0; si++) {
        if (*si == '0')
            *si = 'Z';
        if (*si == 'O')
            *si = 'Y';
        if (*si == 'o')
            *si = 'Y';
    }

    strcat(text, code);
    strupr(text);
}

/*
 * 0x034e2
 *
 * **A score code into a score.** The code is `PASSWORD-XXXXX...`: the password
 * up to the dash, then five hex digits holding the score, then the rest as a
 * base-0x22 checksum.
 *
 * **`Z` and `Y` stand in for `0` and `O`** in what the player types, and are
 * swapped back before anything is parsed - because a printed code with a zero
 * and a letter O next to each other is a code that gets typed in wrong. They
 * are swapped *back again* at the end, so the caller's buffer comes out as the
 * player typed it: `password_to_level` is about to be handed the same string.
 *
 * The checksum is the score multiplied by each of the **first three characters
 * of the password** and the three products added - so a code carries its own
 * password, and one lifted from another player's game does not verify.
 *
 * A code with no dash at all answers 0. One that fails the checksum answers
 * 0xffffffff, which the caller shows a message for; the two are different
 * answers on purpose.
 *
 * Both halves go through `parse_base`, which **reverses what it is given**, so
 * each is copied into a local first. That is why there are two buffers here and
 * not two pointers.
 */
int32_t score_code_to_score(register char *text)
{
    char five[8];                       /* [bp-8], the five score digits */
    char *dash;                         /* [bp-0xa] */
    int16_t i;                          /* [bp-0xc] */
    int32_t score;                      /* [bp-0x10] */
    int32_t check;                      /* [bp-0x14] */
    int32_t sum;                        /* [bp-0x18] */
    char tail[20];                      /* [bp-0x2c], the checksum text */
    register char *si;

    dash = strchr(text, '-');
    if (dash == 0)
        return 0;
    dash++;

    for (si = dash; *si != 0; si++) {
        if (*si == 'Z')
            *si = '0';
        if (*si == 'Y')
            *si = 'O';
    }

    for (i = 0; i < 5; i++)
        five[i] = dash[i];
    five[5] = 0;
    strcpy(tail, dash + 5);

    score = parse_base(five, 0x10);
    check = parse_base(tail, 0x22);

    sum = LONG_MUL(score, (uint8_t)text[0]);
    sum += LONG_MUL(score, (uint8_t)text[1]);
    sum += LONG_MUL(score, (uint8_t)text[2]);

    for (si = dash; *si != 0; si++) {
        if (*si == '0')
            *si = 'Z';
        if (*si == 'O')
            *si = 'Y';
    }

    if (check == sum)
        return score;
    return -1;
}

/*
 * 0x03612
 *
 * **Read a number in an arbitrary base.** Digits are `0`-`9` and then `A`
 * upwards without a limit - subtracting 0x37 from anything at or above `A`
 * gives 10 for `A`, 35 for `Z`, and keeps going past it - which is what lets
 * the caller ask for base 0x22.
 *
 * It **reverses the string first**, in place and permanently, and then
 * accumulates with a `place` that starts at 1 and is multiplied by the base
 * each time round. So the reversal is what makes the first character the most
 * significant; without it the loop would read the number backwards.
 *
 * Nothing validates. A character below `0` yields a negative digit and is
 * accumulated like any other.
 */
int32_t parse_base(register char *text, int16_t base)
{
    int16_t digit;                      /* [bp-2] */
    int32_t total;                      /* [bp-6] */
    int32_t place;                      /* [bp-0xa] */
    int32_t wide;                       /* [bp-0xe], the digit as a long */
    register char *si;

    total = 0;
    place = 1;
    strrev(text);

    for (si = text; *si != 0; si++) {
        if ((uint8_t)*si >= 'A')
            digit = (uint8_t)*si - 0x37;
        else
            digit = (uint8_t)*si - 0x30;
        wide = digit;
        total += LONG_MUL(wide, place);
        place = LONG_MUL(place, base);
    }
    return total;
}
