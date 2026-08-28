/* See dgroup.h: a boundary the port chose, not one the binary proves. */
#include "dgroup.h"

int16_t frame_flag;          /* DGROUP 0x5754 */
int16_t present_hook_a;      /* DGROUP 0x52fa */
int16_t present_hook_b;      /* DGROUP 0x52f2 */

uint8_t  fill_enabled;       /* DGROUP 0x389c */
uint8_t  clip_enabled;       /* DGROUP 0x3893 */
int16_t  clip_left;          /* DGROUP 0x3894 */
int16_t  clip_right;         /* DGROUP 0x3896 */
int16_t  clip_top;           /* DGROUP 0x3898 */
int16_t  clip_bottom;        /* DGROUP 0x389a */
uint8_t  border_colour_a;    /* DGROUP 0x389d */
uint8_t  border_colour_b;    /* DGROUP 0x389e */

uint8_t span_buffer[SPAN_BUFFER_BYTES];   /* the buffer DGROUP 0x4342 names */

int16_t word_4e87;           /* DGROUP 0x4e87 */
