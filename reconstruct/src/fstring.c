/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Far string helpers**: length, copy, bounded copy, and case-blind
 * compares, a character search and a concatenation, on far pointers.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x09e4c..0x0a05f,
 * split out of seg0000.c on 2026-09-27. **Both ends are ours.** Its first
 * four routines are called from nowhere in the image, and 0x09ef8 reserves
 * its one local with `dec sp / dec sp`, which the BC++ 3.0 `-mm -zC_TEXT`
 * modules around it do not write. So these seven look like one module of
 * their own and perhaps another compiler's, but nothing yet proves either.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x09f68
 *
 * **`stricmp` over two far strings.** A null pointer on either side answers 1
 * rather than crashing, and answers it *before* looking at the other, so two
 * nulls compare as unequal too.
 *
 * Each byte goes through `tolower` before it is compared, which is the whole
 * reason this exists rather than `strcmp`: the listing it sorts holds names DOS
 * hands back in capitals and text the game wrote in whatever case it liked.
 *
 * The loop ends on the *first* string's NUL, so the answer for a prefix is the
 * second string's next character negated - and the two pointers are advanced in
 * the caller's own stack slots, not in registers.
 */
int16_t far_stricmp(const char far * a, const char far * b)
{
    int16_t si, di;

    /* **0000:0000, not a C null pointer.** The original's guard is
       `(off | seg) == 0`, and that address is the first byte of the guest's
       memory - written as `b == NULL` it would never fire. */
    if (b == (const char far *)FAR_NULL_PTR
        || a == (const char far *)FAR_NULL_PTR)
        return 1;

    for (;;) {
        si = (int16_t)to_lower((uint8_t)*a++);
        di = (int16_t)to_lower((uint8_t)*b++);

        if (si == 0 || si != di)
            return (int16_t)(si - di);
    }
}

/*
 * 0x09fc0
 *
 * **Borland's `_fstrchr`**, linked in and never called - nothing in the image
 * reaches it by call or by address. A null pointer answers 0000:0000, the
 * guest's null, which is `FAR_NULL_PTR` here and not a C null; searching for
 * NUL itself answers null too, because the loop stops at the terminator and
 * the test after it is for a non-NUL byte.
 */
char far *far_strchr(const char far *s, char c)
{
    if (s == (const char far *)FAR_NULL_PTR)
        return (char far *)FAR_NULL_PTR;

    while (*s != 0 && *s != c)
        s++;

    if (*s != 0)
        return (char far *)s;
    return (char far *)FAR_NULL_PTR;
}

/*
 * 0x0a005
 *
 * **Borland's `_fstrcat`**, linked in and never called. Either pointer null
 * answers null, the source first; the destination is walked to its NUL in a
 * frame copy of the pointer, and the copy includes the terminator. Answers the
 * destination it was given.
 */
char far *far_strcat(char far *dst, const char far *src)
{
    char far *d = dst;                       /* [bp-4]:[bp-2] */
    char c;

    if (src == (const char far *)FAR_NULL_PTR
        || dst == (char far *)FAR_NULL_PTR)
        return (char far *)FAR_NULL_PTR;

    while (*d != 0)
        d++;

    do {
        c = *src++;
        *d++ = c;
    } while (c != 0);

    return dst;
}
