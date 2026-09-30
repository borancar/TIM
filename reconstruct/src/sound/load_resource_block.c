/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Load a block of a resource.**
 *
 * A module of the sound library, in 1.11 **code segment 2ba6** on its own,
 * image 0x2ba6a..0x2bb2c. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2ba6a
 *
 * Load a whole resource into a fresh block and answer it as a far pointer, or
 * null.
 *
 * The resource is opened under the name at DGROUP 0x4a80, its size asked for,
 * a block of exactly that size allocated of the caller's kind, and the whole
 * thing read in. A short read - or any size at all in the high half - frees the
 * block and answers null, so a partial resource is never handed back.
 *
 * The resource is closed on every path that opened it, including the failures.
 *
 * The optional pointer in the fourth argument is filled with the size, but only
 * when there is a block to go with it.
 */
uint8_t far *load_resource_block(FILE *file, uint32_t size,
                                 uint8_t * out, uint16_t kind)
{
    uint32_t len;
    uint8_t far *buf = NULL;
    int16_t handle;

    if ((handle = open_resource(0, file, "r", size)) >= 0) {
        len = resource_size(handle);

        if ((buf = alloc_for_kind(len, kind)) != NULL) {
            /* A size that does not fit in a word never compares equal. */
            if ((uint16_t)read_resource(handle, buf, (uint16_t)len) != len) {
                free_for_kind(buf, kind);
                buf = NULL;
            }
        }

        close_resource(handle);
    }

    if (out != NULL && buf != NULL)
        *(uint32_t *)out = len;

    return buf;
}
