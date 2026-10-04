/*
 * **The routines of Borland's C library that libc has no counterpart for**,
 * on the host. Ours - not transcriptions: under Borland C++ the game links
 * the library's own from CM.LIB, and Borland's library itself, transcribed,
 * is kept in borland/ and not built. See hostlib.h.
 */
#include <ctype.h>
#include <string.h>

#include "hostlib.h"
#include "hostio.h"

/* OURS: `itoa` and `ltoa`, a signed value in base 10 and otherwise the bits
   as unsigned, as Borland's do. */
static char *to_text(uint32_t v, int32_t negative, char *buf, int16_t radix)
{
    char tmp[34];
    char *p = buf;
    int16_t n = 0;

    if (radix < 2 || radix > 36) {
        *buf = 0;
        return buf;
    }
    do {
        uint32_t d = v % (uint32_t)radix;

        tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (uint32_t)radix;
    } while (v != 0);
    if (negative)
        *p++ = '-';
    while (n > 0)
        *p++ = tmp[--n];
    *p = 0;
    return buf;
}

/* OURS: `itoa`. */
char *itoa(int16_t value, char *buf, int16_t radix)
{
    if (radix == 10 && value < 0)
        return to_text((uint32_t)-(int32_t)value, 1, buf, radix);
    return to_text(radix == 10 ? (uint32_t)value : (uint16_t)value, 0, buf, radix);
}

/* OURS: `ltoa`. */
char *ltoa(int32_t value, char *buf, int16_t radix)
{
    if (radix == 10 && value < 0)
        return to_text((uint32_t)0 - (uint32_t)value, 1, buf, radix);
    return to_text((uint32_t)value, 0, buf, radix);
}

/* OURS: `strrev`. */
char *strrev(char *s)
{
    size_t i, n = strlen(s);

    for (i = 0; i < n / 2; i++) {
        char c = s[i];

        s[i] = s[n - 1 - i];
        s[n - 1 - i] = c;
    }
    return s;
}

/* OURS: `strupr`. */
char *strupr(char *s)
{
    char *p;

    for (p = s; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    return s;
}

/* The host's heap is libc's: nothing to check, and no block to walk - so a
   walk ends at once, and `heap_largest_free` answers the room a fresh heap
   has. OURS: both. */
int16_t heapcheck(void)
{
    return _HEAPOK;
}

/* OURS: `heapwalk`. */
int16_t heapwalk(struct heapinfo *info)
{
    (void)info;
    return _HEAPEMPTY;
}

/* OURS: `getdate`, DOS's date, which hostio.c answers. */
void getdate(struct date *d)
{
    uint16_t year, monthday, weekday;

    io_dos_getdate(&year, &monthday, &weekday);
    d->da_year = (int16_t)year;
    d->da_day = (char)(monthday & 0xff);
    d->da_mon = (char)(monthday >> 8);
}

/* The interrupt vector table, which the game fills with its handlers and
   puts back on the way out. On the host nothing dispatches through it - the
   timer and keyboard reach the game's handlers by name - so it is a table
   and no more. Ours. */
static void interrupt (far *vectors[256])();

/* OURS: `getvect`, from the table above. */
void interrupt (far *getvect(uint16_t n))()
{
    return vectors[n & 0xff];
}

/* OURS: `setvect`, into the table above. */
void setvect(uint16_t n, void interrupt (far *handler)())
{
    vectors[n & 0xff] = handler;
}
