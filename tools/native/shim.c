/*
 * The pieces every shim is built from. NOT a transcription; see shim.h.
 */
#include <stdio.h>

#include "shim.h"
#include "../../reconstruct/dgroup.h"

void far_args(call_t *c)  { c->at = c->stack + 4; }
void near_args(call_t *c) { c->at = c->stack + 2; }

static uint16_t peek(uint32_t at)
{
    return (at + 1 < GUEST_MEM_BYTES)
         ? (uint16_t)(guest_mem[at] | (guest_mem[at + 1] << 8)) : 0;
}

uint16_t aword(call_t *c)
{
    uint16_t v = peek(c->at);

    c->at += 2;
    return v;
}

/* Low word first, which is how a `long` sits on the stack. */
uint32_t alng(call_t *c)
{
    uint16_t lo = aword(c), hi = aword(c);

    return ((uint32_t)hi << 16) | lo;
}

/*
 * A far pointer as a host one: offset first, then segment, which is the order
 * `push seg / push off` leaves them in.
 *
 * The conversion is the base and nothing else, because the guest's megabyte
 * *is* `guest_mem` - mapped into the emulator rather than copied. The check is
 * that the address is inside it: a segment:offset reaches 0x10ffef, which is
 * past the end of the array, and a blit handed that would run off it.
 */
const uint8_t *aptr(call_t *c)
{
    uint16_t off = aword(c), seg = aword(c);
    uint32_t at = ((uint32_t)seg << 4) + off;

    if (at >= GUEST_MEM_BYTES) {
        fprintf(stderr, "native: %04x:%04x is outside the guest's memory\n",
                seg, off);
        return guest_mem;
    }
    return guest_mem + at;
}

uint16_t areg(call_t *c, int reg)
{
    uint16_t v = 0;

    uc_reg_read(c->uc, reg, &v);
    return v;
}

static void go_back(call_t *c, uint16_t pops, int far)
{
    uint16_t off = peek(c->stack);
    uint16_t seg = far ? peek(c->stack + 2) : c->cs;
    uint16_t sp  = (uint16_t)(c->sp + (far ? 4 : 2) + pops);

    uc_reg_write(c->uc, UC_X86_REG_SP, &sp);
    uc_reg_write(c->uc, UC_X86_REG_CS, &seg);
    uc_reg_write(c->uc, UC_X86_REG_IP, &off);
}

static void set_ax(call_t *c, uint16_t ax)
{
    uc_reg_write(c->uc, UC_X86_REG_AX, &ax);
}

void rf_void(call_t *c, uint16_t pops) { go_back(c, pops, 1); }
void rn_void(call_t *c, uint16_t pops) { go_back(c, pops, 0); }

void rf_ax(call_t *c, uint16_t ax, uint16_t pops)
{
    set_ax(c, ax);
    go_back(c, pops, 1);
}

void rn_ax(call_t *c, uint16_t ax, uint16_t pops)
{
    set_ax(c, ax);
    go_back(c, pops, 0);
}

/* Borland answers a 32-bit value in DX:AX - the high half in DX. */
static void set_dxax(call_t *c, uint32_t v)
{
    uint16_t ax = (uint16_t)v, dx = (uint16_t)(v >> 16);

    uc_reg_write(c->uc, UC_X86_REG_AX, &ax);
    uc_reg_write(c->uc, UC_X86_REG_DX, &dx);
}

void rf_dxax(call_t *c, uint32_t v, uint16_t pops)
{
    set_dxax(c, v);
    go_back(c, pops, 1);
}

void rn_dxax(call_t *c, uint32_t v, uint16_t pops)
{
    set_dxax(c, v);
    go_back(c, pops, 0);
}
