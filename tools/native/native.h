/*
 * The hybrid runner: the original binary under emulation, with the port's
 * proven C standing in for the hardware and, routine by routine, for the code.
 *
 * NOT a transcription. Nothing here corresponds to anything in TIM.EXE; it is
 * the seam between the emulator and `reconstruct/`, and it exists so a
 * transcribed routine can be exercised **inside the real game** rather than
 * only against a captured call.
 *
 * The arrangement, and why it is this way round:
 *
 *   - `guest_mem` is mapped into the emulator with `uc_mem_map_ptr`, so the
 *     guest and the port address the same bytes. No copying, no seeding, no
 *     drift. `dgroup_base` already defaults to 0x2e4c0, which is the load
 *     segment the emulator uses, so the two agree without being told.
 *
 *   - The VGA is the port's. Reads and writes of the A000 aperture and every
 *     `in`/`out` are routed into `reconstruct/io.c`, so the plane model that
 *     draws the port's frames is the one the emulated game draws into. That is
 *     the "native graphics layer" this program exists to provide.
 *
 *   - **No DOS is implemented here.** The port's io layer already has it. The
 *     guest is meant never to execute `int 21h`, because the Borland routines
 *     that would issue it are dispatched natively instead. An interrupt that
 *     does fire is therefore not a missing feature - it is a routine that has
 *     not been dispatched yet, and it stops the run with a guest backtrace
 *     saying which one.
 */
#ifndef NATIVE_H
#define NATIVE_H

#include <stdint.h>
#include <unicorn/unicorn.h>

/* How the callee returns, and what it leaves behind. */
enum { RET_NONE = 0, RET_AX, RET_DXAX };

/*
 * One routine the emulator should not execute, because the port has it.
 *
 * `at` is an image offset for code in TIM.EXE. For the video driver it is an
 * offset into VM.OVL, whose segment the loader chooses at run time; `overlay`
 * says which, and `native_bind_overlay` fills the linear address in once the
 * game has loaded the driver.
 *
 * `nargs` counts **words on the stack**, which is what almost every routine
 * here takes; `pops` is what a `ret N` removes, and is zero for the ordinary
 * C convention where the caller cleans up. Getting either wrong desynchronises
 * the guest's stack, so both are read from the routine rather than assumed.
 */
typedef struct {
    uint32_t     at;
    const char  *name;
    void        *fn;
    /*
     * Where the arguments are. NULL means the stack, which is most of them;
     * otherwise it is the registers, in the order the port's function takes
     * them, and `nargs` counts those instead of stack words.
     *
     * The emulator has the registers at the moment the block hook fires, which
     * is the routine's own entry - exactly where the original would have read
     * them - so nothing has to be reconstructed. The polygon filler needs this:
     * `poly_walk` takes x in ax, frac in bx, step in si, acc in bp, count in
     * cx, the offset in di and the segment in ES, and none of it is on the
     * stack.
     */
    const int   *regs;
    uint8_t      overlay;      /* 0 = image, 1 = VM.OVL */
    uint8_t      far_call;     /* 1 if the routine ends `retf` */
    uint8_t      nargs;        /* stack words the port function takes */
    uint8_t      pops;         /* bytes a `ret N` removes; 0 for cdecl */
    uint8_t      ret;          /* RET_NONE / RET_AX / RET_DXAX */
    uint32_t     linear;       /* resolved address, 0 until bound */
    uint32_t     hits;
} native_fn;

extern native_fn native_table[];
extern int32_t   native_count;

/* Resolve every image-relative entry once the program is loaded. */
void     native_bind_image(void);
/* Resolve the VM.OVL entries once the driver's segment is known; 0 if not. */
int32_t  native_bind_overlay(uc_engine *uc);
/* The routine registered at this linear address, or NULL. */
native_fn *native_lookup(uint32_t linear);
/* Run one natively and put the guest back where the `call` would have. */
void     native_call(uc_engine *uc, native_fn *f);

/*
 * The guest's own call chain, printed from its BP frames.
 *
 * Borland's large model builds a frame in every routine that takes arguments -
 * `push bp / mov bp,sp` - so the chain is walkable without tracking every call
 * and every `ret`, which would cost a hook on every instruction. Where a frame
 * is missing the walk stops rather than inventing one.
 */
void     guest_backtrace(uc_engine *uc, const char *why);
/* The transcribed routine containing this image offset, or NULL. */
const char *sym_for(uint32_t image_off, uint32_t *start);

#endif
