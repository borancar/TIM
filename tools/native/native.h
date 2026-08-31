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

#endif
