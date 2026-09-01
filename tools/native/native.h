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

/* Resolve every image-relative entry once the program is loaded. */
void     native_bind_image(void);
/* Resolve the VM.OVL entries once the driver's segment is known; 0 if not. */
int32_t  native_bind_overlay(uc_engine *uc);
/* Run the port's routine if one is registered here; 1 if it did. */
int32_t  native_dispatch(uc_engine *uc, uint32_t linear);
int32_t  native_count_routines(void);
void     native_report(void);

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
/* The routine that *starts* at this offset, or -1; and the table by index. */
int32_t     sym_index(uint32_t image_off);
int32_t     sym_count_of(void);
uint32_t    sym_at(int32_t i);
const char *sym_name(int32_t i);

/*
 * ---------------------------------------------------------------------------
 * Taking a routine off the queue
 *
 * `tools/native/coverage.py` lists the routines a stretch of the game runs
 * that the port already has a body for. That list is long and mostly cold, so
 * do not work it in order. The runner prints where the guest is every four
 * hundred slices; count those and dispatch what the count names. One routine
 * was 92 of 145 samples and worth more than the six tranches before it.
 *
 * Then, for each candidate, **read it**. Four facts, none of them inferable
 * from the routine next door:
 *
 *   far or near     `retf` or `ret` - but read the *entry* too. `pop bx /
 *                   push cs / push bx` is a thunk that makes a far frame for
 *                   a near caller, and both its exits are `retf`. genshims.py
 *                   refuses that one now; nothing catches the others.
 *   who pops        a bare `retf`, or `retf N` with the callee cleaning up.
 *   the arguments   count the words at `[bp+6]`, or the registers if there is
 *                   no frame at all. `mov bx,sp` and `push bp / push si` are
 *                   both entries without one.
 *   the answer      AX, DX:AX, or nothing - from the port's prototype.
 *
 * Bound the routine by the next symbol rather than by a window you picked:
 * a return past the end of what you read looks exactly like a routine that
 * has none, and `apply_contact_friction` was left on the queue an extra day
 * for that.
 *
 * Then `make` - checking its exit status, not its output, because a generator
 * that throws still leaves yesterday's shims.c on disk - and run
 * `tools/check_native.py`. One tranche at a time: when six went in together
 * the intro broke and bisecting cost more than six separate runs would have.
 *
 * A routine whose callers are all dispatched can never be exercised again,
 * and a screen that passes says nothing about it. Say so where you declare it.
 *
 * **Where the queue stops.** Not at zero. Work it by call count and the tail
 * arrives at routines called exactly once, and those are the startup path -
 * `game_main`, `game_startup`, `game_intro`, `load_all_parts`. Dispatching
 * `game_main` would take the whole program with it: its callees leave the
 * emulator too, the run becomes the port calling itself, and the thing that
 * makes this runner worth having - the *original's* code executing beside the
 * port's, disagreeing where the port is wrong - is gone. A hybrid that
 * dispatches its entry point is not a hybrid.
 *
 * So the sensible end is a top-level shell still emulated with everything
 * below it dispatched, which is roughly where it stands: 55 routines entered,
 * fifty of them once each, and five above a single call.
 * ---------------------------------------------------------------------------
 */

#endif
