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
 *
 *   - **The window is the port's**, `reconstruct/sdl.c`, opened here the way
 *     main.c opens it: the same grab, the same Ctrl+Alt to hand the pointer
 *     back, the same Escape to quit. So the hybrid is playable, and what plays
 *     it is the port's code. This ran headless for a long time and composed
 *     frames for tools/ to read, which proves the pixels agree and says
 *     nothing about whether the game can be played at all.
 *
 * Steering it, all through the environment - there are no options, because
 * `main` takes none:
 *
 *   TIM_DIR=<dir>                   where TIM.img and TIM.unpacked.exe are
 *   TIM_HEADLESS=1                  no window; what check_native.py runs
 *   TIM_STOP=<frame>                run to that frame and stop cleanly
 *   TIM_CLICK=<frame>:<x>:<y>,...   clicks, to get past screens that wait
 *   TIM_FRAMES=<dir>:<step>[:<from>:<to>]   dump frames; 308 KB each
 *   TIM_FRAME=<frame>:<path>        one frame
 *   TIM_FRAMEHASH=<path>[:<from>:<to>]      32 bytes a frame instead
 *   TIM_ENTRIES=<path>              what the *original* still executes
 *
 * And the snapshots, which are how anything behind the menu gets reached at
 * all. The intros are all a run from the entry point gets to on its own; the
 * menu, the editor and the game proper are behind a person pressing keys, and
 * reaching them by hand for every measurement is what leaves them untested.
 * Play once, capture, start there from then on:
 *
 *   Shift+F2                        capture, at the next call out of the guest
 *   TIM_SNAPDIR=<dir>               where captures go (default out/)
 *   TIM_SNAP=<path>                 one fixed file instead, numbering off
 *   TIM_SNAPAT=<frame>              arm from the clock instead of the key
 *   TIM_RESTORE=<path>              start as that machine, not at the entry
 *
 * Captures are numbered `snap001.snap`, `snap002.snap` and so on, and the
 * **numbering is shared with the port** - `tim` and `devtim` take theirs the
 * same way, so a session's captures come out in the order they were made
 * whichever binary made them. The number is found by looking rather than
 * remembered, so it survives a restart and never overwrites an earlier run.
 *
 * The two kinds are told apart by what is inside, not by the name: a port
 * capture begins "TIMPORT1", a runner capture with this file's own magic.
 * Only a runner capture can be restored - the port is C and has no CPU state
 * to put back - but the megabyte of memory in either compares with the
 * megabyte in the other, which is what they are for.
 *
 * **Not interchangeable with tools/snapshot.py's.** That one captures the
 * Python emulator; this one captures guest memory, the emulator's registers
 * and `reconstruct/io.c`'s state, which is a different machine.
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
/* Take the snapshot Shift+F2 armed, if it did; called at every dispatch. */
void     native_snapshot_if_armed(uc_engine *uc, const char *at);
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
 * What has been made to fail on purpose
 *
 * Every safety mechanism here has been fired deliberately, because one that
 * has only ever been quiet is indistinguishable from one that does not work.
 *
 *   the screen check      one pixel altered in 307,200 named the two flips
 *                         carrying it and returned 1; and again, one bit of
 *                         one digest after the comparison moved to digests.
 *   the int 21h trap      withdrawing dos_alloc_bytes stops at `int 21h
 *                         (ah=48)` and walks dos_alloc_bytes+0x5f ->
 *                         game_startup -> game_main -> entry.
 *   the VGA port trap     withdrawing vm_fill_spans stops at `wrote VGA port
 *                         3ce` and names VM.OVL VGA:0x0bec vm_fill_spans+0x6
 *                         with game_intro+0xf6 above it.
 *   the thunk guard       putting a far declaration back on long_shift_left
 *                         stops the build with the register named.
 *   the A000 hook         **cannot** fire - see native.c. Every path into
 *                         video memory sets the graphics controller first and
 *                         the port trap catches that. Its silence is not
 *                         coverage.
 *
 * When one of these changes, fire it again. A proof of one comparison says
 * nothing about the comparison that replaced it, which is why the screen check
 * is in that list twice.
 * ---------------------------------------------------------------------------
 */

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
 * Then, for each candidate, **read it**. `tools/native/conv.py <addr> ...`
 * does the mechanical part and prints the routines.def line - near or far from
 * the return, the argument count from the port's prototype, the pops from
 * `ret N`, and a warning if the frame offsets contradict the frame kind. It
 * narrows the reading; it does not finish it, and its own docstring lists what
 * it cannot see. Four facts, none of them inferable from the routine next
 * door:
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
