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
 * split out of seg0000.c on 2026-09-27. **Built with Borland C++ 2.0**, not
 * the 3.0 of the modules around it: only BC++ 2.0 `-mm` gives all seven
 * (BC++ 3.0 matches one and Turbo C++ 1.0x six). A second compiler is a
 * module boundary on each side, so both ends are the original's here, not
 * ours. `-zC_TEXT` because the code is in `_TEXT`.
 *
 * Its first four routines are called from nowhere in the image, so the
 * port never had them; they are transcribed because the module holds them.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include <ctype.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x09e4c
 *
 * **The length of a far string**, 0 for a null pointer.
 *
 * Nothing in the image calls this or the three after it, near or far; they
 * are transcribed because the module holds them. The names are ours.
 */
int16_t far_strlen(const char far *s)
{
    register int16_t si;

    si = 0;
    if (s != (const char far *)NULL)
        while (*s != 0) {
            si++;
            s++;
        }
    return si;
}

/*
 * 0x09e70
 *
 * **Copy a far string**, and answer where the copy *ended* - one past the
 * terminator - not where it began. A null source or destination copies
 * nothing and answers the destination.
 */
char far *far_strcpy(char far *dst, const char far *src)
{
    if (src != (const char far *)NULL && dst != (char far *)NULL)
        while ((*dst++ = *src++) != 0)
            ;
    return dst;
}

/*
 * 0x09ea5
 *
 * **Copy at most `n` characters of a far string** and terminate the copy,
 * answering where the terminator went; a null pointer answers null.
 */
char far *far_strncpy(char far *dst, const char far *src, register int16_t n)
{
    if (src == (const char far *)NULL || dst == (char far *)NULL)
        return (char far *)NULL;
    while (*src != 0 && n--) {
        *dst = *src;
        src++;
        dst++;
    }
    *dst = 0;
    return dst;
}

/*
 * 0x09ef8
 *
 * **Compare at most `n` characters of two far strings, case-blind**, the
 * difference of the first two that differ; 1 if either is null.
 */
int16_t far_strnicmp(const char far *a, const char far *b, register uint16_t n)
{
    register int16_t si;
    int16_t d;                          /* [bp-2] */

    si = 0;
    d = 0;
    if (b == (const char far *)NULL || a == (const char far *)NULL)
        return 1;
    if (n != 0)
        do {
            si = tolower((uint8_t)*a++);
            d = tolower((uint8_t)*b++);
        } while (--n != 0 && si != 0 && si == d);
    return si - d;
}

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

int16_t far_stricmp(const char far *a, const char far *b)
{
    register int16_t si;
    register int16_t di;

    si = 0;
    di = 0;
    /* **0000:0000, not a C null pointer.** The original's guard is
       `(off | seg) == 0`, and that address is the first byte of the guest's
       memory - written as `b == NULL` it would never fire. */
    if (b == (const char far *)NULL || a == (const char far *)NULL)
        return 1;
    do {
        si = tolower((uint8_t)*a++);
        di = tolower((uint8_t)*b++);
    } while (si != 0 && si == di);
    return si - di;
}

/*
 * 0x09fc0
 *
 * **Borland's `_fstrchr`**, linked in and never called - nothing in the image
 * reaches it by call or by address. A null pointer answers 0000:0000, the
 * guest's null, which is `NULL` here and not a C null; searching for
 * NUL itself answers null too, because the loop stops at the terminator and
 * the test after it is for a non-NUL byte.
 */

char far *far_strchr(const char far *s, char c)
{
    if (s == (const char far *)NULL)
        return (char far *)NULL;
    while (*s != 0 && *s != c)
        s++;
    if (*s != 0)
        return (char far *)s;
    else
        return (char far *)NULL;
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
    char far *d;                        /* [bp-4] */

    d = dst;
    if (src == (const char far *)NULL || dst == (char far *)NULL)
        return (char far *)NULL;
    while (*d != 0)
        d++;
    while ((*d++ = *src++) != 0)
        ;
    return dst;
}
