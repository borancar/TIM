/*
 * Borland's DOS file primitives, from the C runtime at the top of segment 0000.
 *
 * Same standing as `borland_heap.c`: **not the game**, not what the port is
 * reconstructing, and kept rather than deleted - these are the runtime's, so
 * having them transcribed and checked against a real binary is worth something
 * for any other Turbo C or Borland C++ DOS program.
 *
 * They are here because the resource loader cannot be verified without them.
 * Every route through it - even the one that reads from the packed archive -
 * ends in the runtime's stdio, and that in turn ends here.
 *
 * The DOS calls themselves go to `io_dos_read` and `io_dos_lseek`, which serve
 * the game's own files read-only. See io.c.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * 0x0c185
 *
 * `read`. INT 21h AH=3Fh, into a **near** buffer - the count and the pointer
 * are both single words, so a read cannot cross a segment.
 *
 * Before it asks DOS it checks the handle's entry in the flag table at DGROUP
 * 0x4d06, two bytes per handle, and refuses with errno 5 if bit 1 is set. That
 * is how a handle opened write-only is stopped without DOS being troubled.
 *
 * The failure path hands the DOS error code to `__IOerror`, which is not
 * transcribed - a read that fails is not something these screens do.
 */
int16_t dos_read(int16_t handle, uint16_t buf, uint16_t count)
{
    int16_t got;

    if ((DGU16(0x4d06 + 2 * handle) & 2) != 0) {
        not_transcribed("__IOerror after a read refused by the handle flags");
        return -1;
    }

    got = io_dos_read(handle, dgroup + buf, count);
    if (got < 0) {
        not_transcribed("__IOerror after a failed DOS read");
        return -1;
    }
    return got;
}

/*
 * 0x0c0c3
 *
 * `lseek`. INT 21h AH=42h with the direction in AL, answering the new position
 * in DX:AX.
 *
 * It clears **bit 9** of the handle's flag word first. That bit says the
 * buffered layer above has something pushed back; seeking throws that away, and
 * clearing it here rather than in the caller is what stops a pushed-back byte
 * surviving a seek and being read at the wrong offset.
 */
int32_t dos_lseek(int16_t handle, uint16_t lo, uint16_t hi, int16_t whence)
{
    int32_t pos;

    DG16(0x4d06 + 2 * handle) = (int16_t)(DGU16(0x4d06 + 2 * handle) & 0xfdff);

    pos = io_dos_lseek(handle, (int32_t)(((uint32_t)hi << 16) | lo), whence);
    if (pos < 0) {
        not_transcribed("__IOerror after a failed DOS seek");
        return -1;
    }
    return pos;
}
