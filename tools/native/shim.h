/*
 * What a shim is written in terms of: the guest's arguments, and its return.
 *
 * NOT a transcription. A shim mirrors the call Borland would have compiled -
 * one parameter per word the guest pushed, converted to what the port's
 * function takes - and then puts the machine back the way the routine's own
 * `ret` would have left it. These are the pieces it does that with.
 *
 * The arguments are read in order, because that is how they are on the stack
 * and it is the only order that survives a routine taking a mixture of widths:
 * a word is one, a far pointer is two, a `long` is two. Reading them with a
 * cursor rather than by index means a pointer in the middle cannot silently
 * shift everything after it, which is exactly the mistake a table of counts
 * allowed.
 */
#ifndef SHIM_H
#define SHIM_H

#include <stdint.h>
#include <unicorn/unicorn.h>

typedef struct {
    uc_engine *uc;
    uint16_t   ss, sp, cs;
    uint32_t   stack;      /* linear address of the pushed return address */
    uint32_t   at;         /* cursor: the next argument word */
} call_t;

/* Where the arguments start: past the return address, which is four bytes for
 * a far call and two for a near one. */
void far_args(call_t *c);
void near_args(call_t *c);

uint16_t        aword(call_t *c);
uint32_t        alng(call_t *c);
const uint8_t  *aptr(call_t *c);
uint16_t        areg(call_t *c, int reg);
/* A far pointer held in a register pair, and the carry flag as 0 or 1 - one
 * of the driver's blitters takes its direction that way. */
const uint8_t  *aregptr(call_t *c, int seg_reg, int off_reg);
uint32_t        acarry(call_t *c);

/*
 * The return, as the routine's own would have left it: the pushed address
 * gone, `pops` more bytes gone if it is one of the pascal ones, the answer in
 * AX or DX:AX, and CS:IP back at the caller.
 */
void rf_void(call_t *c, uint16_t pops);
void rf_ax(call_t *c, uint16_t ax, uint16_t pops);
void rf_dxax(call_t *c, uint32_t dxax, uint16_t pops);
void rn_void(call_t *c, uint16_t pops);
void rn_ax(call_t *c, uint16_t ax, uint16_t pops);
void rn_dxax(call_t *c, uint32_t dxax, uint16_t pops);

/* One dispatched routine: where it is, and the shim that stands in for it. */
typedef struct {
    uint32_t     at;          /* image offset, or an offset into VM.OVL */
    const char  *name;
    void       (*shim)(call_t *c);
    uint8_t      overlay;
} shim_entry;

extern const shim_entry shim_table[];
extern const int32_t    shim_count;

#endif
