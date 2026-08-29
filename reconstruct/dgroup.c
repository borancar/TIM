/*
 * Storage for the original's DGROUP and for the span-list buffer. See
 * dgroup.h for why DGROUP is an array rather than a set of named globals.
 */
#include "dgroup.h"

uint8_t  guest_mem[GUEST_MEM_BYTES];

/*
 * Where DGROUP sits in that megabyte. The original's loader decides it - 0x110
 * paragraphs for the program, so DGROUP lands at 0x2e4c0 - and tools/verify.py
 * sets it from the run it captured, so the segment values held in the game's
 * own data mean the same thing on both sides.
 */
uint32_t dgroup_base = 0x2E4C0;

/*
 * The port's own stack pointer - see dgroup.h. The default is the top of
 * DGROUP; tools/verify.py replaces it with the original's SP for each call it
 * compares.
 */
uint16_t guest_sp = 0xFF9E;

/*
 * NOT a transcription: the port's own frame reservation. The original has no
 * such routine - it says `push bp / mov bp,sp / sub sp,0x40` - so there is no
 * address to point at. See dgroup.h for why the port needs a stack at all.
 *
 * Pass the **whole** frame below BP - the locals `sub sp` reserves and any
 * registers the prologue pushes after it - not just the locals. A routine whose
 * own locals nothing looks at still has to reserve, if it calls one whose
 * locals are looked at: otherwise the callee's frame lands inside the caller's
 * and every address it hands out is wrong.
 *
 * It reserves **two bytes more** than asked for, because the saved BP sits
 * between the caller's SP and the locals: with the frame `sub sp,N` builds, the
 * local at `bp-N` is at entry-SP minus N minus 2. Getting that wrong puts every
 * local two bytes high, which is invisible for a local nothing outside the
 * routine sees and shows up at once for one whose address is handed to another
 * routine - which is the only reason any of this exists.
 */
uint16_t dg_enter(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp - bytes - 2);
    return guest_sp;
}

/*
 * NOT a transcription: the bytes a **call** itself puts on the stack - the
 * arguments and the return address - which the port passes in C and so never
 * pushes.
 *
 * Only a caller of a routine that hands out the addresses of its own locals
 * needs this. For everyone else the guest stack is invisible and where it
 * happens to be does not matter; for those, the callee's locals land eight or
 * so bytes high without it, which is exactly the kind of error that agrees with
 * every check until one of those addresses reaches DGROUP.
 */
void dg_call(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp - bytes);
}

/*
 * NOT a transcription either: the counterpart of `dg_call`, giving back what
 * the call put on the stack.
 */
void dg_uncall(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp + bytes);
}

/*
 * NOT a transcription either, and the counterpart of the `mov sp,bp / pop bp`
 * the original's epilogue does. It gives back what dg_enter took, the saved
 * BP's two bytes included.
 */
void dg_leave(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp + bytes + 2);
}
