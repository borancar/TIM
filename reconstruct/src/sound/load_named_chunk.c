/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Load a chunk by its name out of a resource.**
 *
 * A module of the sound library, in 1.11 **code segment 288a** on its own,
 * image 0x288aa..0x28935. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: compiler bc3.10
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x288aa
 *
 * Load a named chunk out of a file, and answer it as a far pointer or null.
 *
 * The file may arrive as a handle or as a name: `file_record_valid` decides
 * which, and a name is opened here - the flag in the first local remembering
 * that this routine owns it and has to close it again.
 *
 * `seek_named_chunk` positions the file at the chunk and is refused on -1:-1.
 * `file_record_size` then answers the size, and it is handed straight to
 * `load_resource_block` as the size to open with - and the two values pushed
 * before it, a kind of 1 and a null out-pointer, are left on the stack across
 * that call, which is why only two bytes are cleaned after the size.
 *
 * A file this routine opened is closed on every path, including the ones that
 * give up; one it was handed is left alone.
 */
uint8_t far *load_named_chunk(char *name, const char * path,
                              uint16_t index)
{
    int16_t opened = 0;
    uint8_t far *r = NULL;
    FILE *si;

    /* A handle, or a name to open. */
    if (file_record_valid((FILE *)name) == 0) {
        opened = 1;
        si = open_file_record(name);
    } else {
        si = (FILE *)name;
    }

    if (si != 0 && seek_named_chunk(si, path, (int16_t)index) != -1L)
        r = load_resource_block(si, file_record_size(si), NULL, 1);

    if (opened != 0)
        close_file_record(si);

    return r;
}
