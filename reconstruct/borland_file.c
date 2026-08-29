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

/*
 * 0x0d0ed
 *
 * The buffered read under `fread`: fill `count` bytes from a `FILE` and answer
 * how many were **not** filled. A near routine with a callee-cleaned frame -
 * `ret 6` - so its three arguments are the `FILE`, the count and the buffer.
 *
 * The `FILE` fields it uses are +0 the bytes left in the buffer, +2 the flags,
 * +4 the DOS handle, +6 the buffer size and +0xa the read pointer.
 *
 * Three ways a byte arrives, and which one runs is worth knowing because they
 * differ enormously in cost. Measured over a run: 16,468 calls, of which 44
 * reach DOS and 395 reach `getc` - **97% are served entirely from the buffer
 * already in hand**.
 *
 * The DOS path is taken only when the request is at least a whole buffer and
 * the buffer is empty, and it then reads as many whole buffers as fit, straight
 * into the caller's memory without going through the buffer at all. A short
 * read sets the error flag 0x20 and gives up.
 *
 * Otherwise bytes come one at a time: from the buffer while +0 lasts, and
 * through `getc` when it runs out. **`getc` is not transcribed**, so that path
 * refuses - and reaching it is what stops this routine and `fread` above it
 * being verified at all: the occurrences the harness samples include one.
 *
 * `getc` itself is 0x0d3ef onto 0x0d404, whose own fast path is just a byte out
 * of the buffer - but it is never reached from here, because this routine only
 * calls it once the buffer is empty. What is needed is the refill beneath it:
 * 0x0d396, 0x0d36d, 0x0da6d, 0x0cd9e, 0x0ce92 and 0x0bfcd. 0x0da6d is the one
 * that reaches `dos_read`, which is already verified, so the chain has ground
 * under it.
 *
 * The count is incremented at the head of the loop and decremented in the body,
 * which nets to nothing on the way in and is what lets the same decrement serve
 * both the first pass and every byte after it. Written out as the original has
 * it rather than tidied, because the balance is easy to break.
 */
uint16_t buffered_read(uint16_t file, uint16_t count, uint16_t buf)
{
    uint16_t di;
    /*
     * The original does not initialise DX. On the first pass the test at the
     * end of the loop reads whatever the caller left in it - and every caller
     * is `fread`, which leaves the high word of its own size-times-count, so it
     * is zero. Written as zero here rather than left to chance.
     */
    uint16_t dx = 0;

    goto test;

loop:
    count++;

    di = *(uint16_t *)(dgroup + file + 6);
    if (di > count)
        di = count;

    if ((DGU16(file + 2) & 0x40) != 0 && DGU16(file + 6) != 0
        && DGU16(file + 6) < count && DGU16(file) == 0) {
        count--;
        di = 0;
        while (DGU16(file + 6) <= count) {
            di = (uint16_t)(di + DGU16(file + 6));
            count = (uint16_t)(count - DGU16(file + 6));
        }

        dx = (uint16_t)dos_read((int16_t)DG8(file + 4), buf, di);
        buf = (uint16_t)(buf + dx);
        if (dx == di)
            goto test;

        count = (uint16_t)(count + (di - dx));
        goto set_error;
    }

next_byte:
    count--;
    if (count == 0)
        goto check_eof;
    di--;
    if (di == 0)
        goto check_eof;

    DG16(file)--;
    if (DG16(file) < 0) {
        not_transcribed("0x0d3ef, getc - the byte-at-a-time refill");
        dx = 0xffff;
    } else {
        uint16_t p = DGU16(file + 0xa);

        DG16(file + 0xa) = (int16_t)(p + 1);
        dx = DG8(p);
    }

    if (dx != 0xffff) {
        DG8(buf) = (uint8_t)dx;
        buf++;
        goto next_byte;
    }

check_eof:
    if (dx == 0xffff)
        goto set_error;

test:
    if (count != 0)
        goto loop;
    return count;

set_error:
    DG16(file + 2) = (int16_t)(DGU16(file + 2) | 0x20);
    return count;
}

/*
 * 0x0d1c4
 *
 * `fread`. Answers how many whole items were read, not how many bytes.
 *
 * A size of zero answers zero without touching the file. The product of size
 * and count is worked out in 32 bits and a request of more than 0xffff bytes is
 * refused outright rather than truncated - so a single `fread` can never ask
 * for more than a segment.
 *
 * The division at the end is what turns bytes into items, and it means a
 * partial item at the end of a file is **not** reported: reading three and a
 * half records answers three.
 */
uint16_t stdio_fread(uint16_t buf, uint16_t size, uint16_t count,
                     uint16_t file)
{
    uint32_t total;
    uint16_t left;

    if (size == 0)
        return 0;

    total = (uint32_t)size * count;
    if (total > 0xffff)
        return 0;

    left = buffered_read(file, (uint16_t)total, buf);
    return (uint16_t)(((uint16_t)total - left) / size);
}

/*
 * 0x0da6d
 *
 * The layer between `read` and DOS: validate the handle, read, and translate
 * line endings if the handle is in text mode.
 *
 * The handle is checked against `_nfile` at DGROUP 0x4d04 and refused with
 * errno 6 above it. A count of 0 or 0xffff answers zero without reading, and so
 * does bit 9 of the handle's flags - the end-of-file mark this routine sets
 * itself when it meets a 0x1a.
 *
 * Text mode is bit 0x4000 of the flags. In it, carriage returns are dropped and
 * a 0x1a ends the file: the routine **seeks back** so the next read starts just
 * after it, and sets bit 9 so nothing reads past. It also refills when
 * translation leaves nothing, which is why a text-mode read of a file of bare
 * carriage returns loops rather than returning zero.
 *
 * **None of that runs here.** The game opens with "rb", so the flag is clear
 * and the translation is dead - transcribed as the refusal it is rather than
 * written on faith, since nothing on these screens can check it.
 */
int16_t read_translated(int16_t handle, uint16_t buf, uint16_t count)
{
    int16_t got;

    if ((uint16_t)handle >= DGU16(0x4d04)) {
        not_transcribed("__IOerror after a read on a handle above _nfile");
        return -1;
    }

    if ((uint16_t)(count + 1) < 2
        || (DGU16(0x4d06 + 2 * handle) & 0x200) != 0)
        return 0;

    got = dos_read(handle, buf, count);

    if ((uint16_t)(got + 1) < 2
        || (DGU16(0x4d06 + 2 * handle) & 0x4000) == 0)
        return got;

    not_transcribed("0x0da6d's text-mode translation, which \"rb\" never uses");
    return -1;
}
