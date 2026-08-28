/*
 * Storage for the original's DGROUP and for the span-list buffer. See
 * dgroup.h for why DGROUP is an array rather than a set of named globals.
 */
#include "dgroup.h"

uint8_t dgroup[DGROUP_BYTES];
uint8_t span_buffer[SPAN_BUFFER_BYTES];   /* the buffer DGROUP 0x4342 names */
