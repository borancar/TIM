/*
 * **The routines of Borland's C library that libc has no counterpart for.**
 * Ours, and host-only: under Borland C++ they are the library's own, declared
 * by <stdlib.h>, <string.h>, <alloc.h> and <dos.h>, and linked from CM.LIB.
 * A game file that calls one includes the Borland header under `__TURBOC__`
 * and this on the host. Everything else the game calls is libc's.
 */
#ifndef HOSTLIB_H
#define HOSTLIB_H

#include <stdint.h>
#include <strings.h>

#include "dgroup.h"

/* <stdlib.h> */
char *itoa(int16_t value, char *buf, int16_t radix);
char *ltoa(int32_t value, char *buf, int16_t radix);

/* <string.h> */
#define stricmp  strcasecmp
#define strnicmp strncasecmp
char *strrev(char *s);
char *strupr(char *s);

/* <alloc.h>, the medium model's: the near heap, walked a block at a time.
   The host's heap is libc's and cannot be walked, so the walk finds none. */
struct heapinfo {
    void    *ptr;
    uint16_t size;
    int16_t  in_use;
};
#define _HEAPEMPTY  1
#define _HEAPOK     2
#define _HEAPEND    5
int16_t heapcheck(void);
int16_t heapwalk(struct heapinfo *info);

/* <dos.h> */
struct date {
    int16_t da_year;
    char    da_day;
    char    da_mon;
};
void getdate(struct date *d);
struct far_ptr getvect(uint16_t n);
void setvect(uint16_t n, struct far_ptr handler);

#endif
