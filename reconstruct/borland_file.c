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
 * through `getc` when it runs out - which is where the refill happens, so this
 * loop never has to know a buffer exists.
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
        dx = (uint16_t)stdio_getc(file);
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

/*
 * 0x0d36d
 *
 * Flush every stream that has something to flush: the twenty `FILE` structures
 * from DGROUP 0x4bc4, sixteen bytes apart, taking those whose flags have
 * **both** 0x100 and 0x200 set.
 *
 * The count is walked down rather than up, and the test is on the value before
 * the decrement, so the last structure examined is the first in the table.
 *
 * The flush itself, 0x0ce92, is not transcribed: the game only reads, so no
 * stream here ever has both bits.
 */
void flush_all_streams(void)
{
    uint16_t si = 0x4bc4;
    int16_t n;

    for (n = 0x14; n != 0; n--) {
        if ((DGU16(si + 2) & 0x300) == 0x300)
            not_transcribed("0x0ce92, the stream flush - the game only reads");
        si = (uint16_t)(si + 0x10);
    }
}

/*
 * 0x0d396
 *
 * Refill a `FILE`'s buffer, and answer 0 or -1. A near routine with a
 * callee-cleaned frame - `ret 2`.
 *
 * A stream marked 0x200 flushes every other stream first. That is what stops a
 * program reading stale data back out of a file something else has written and
 * not yet flushed, and it is why a read can cost a write.
 *
 * The read pointer at +0xa is reset to the buffer base at +8 **before** the
 * read, so the bytes land where the pointer already points.
 *
 * Nothing read is told apart two ways: a count of exactly zero is end of file,
 * setting 0x20 and clearing the 0x180 pair, and anything else is an error,
 * setting 0x10 and forcing the count to zero. Both answer -1, so the caller
 * cannot tell them apart from the answer alone - it has to look at the flags.
 */
int16_t refill_stream(uint16_t file)
{
    int16_t got;

    if ((DGU16(file + 2) & 0x200) != 0)
        flush_all_streams();

    DG16(file + 0xa) = DG16(file + 8);

    got = read_translated((int16_t)DG8(file + 4), DGU16(file + 8),
                          DGU16(file + 6));
    DG16(file) = got;

    if (got > 0) {
        DG16(file + 2) = (int16_t)(DGU16(file + 2) & 0xffdf);
        return 0;
    }

    if (DG16(file) == 0)
        DG16(file + 2) = (int16_t)((DGU16(file + 2) & 0xfe7f) | 0x20);
    else {
        DG16(file) = 0;
        DG16(file + 2) = (int16_t)(DGU16(file + 2) | 0x10);
    }
    return -1;
}

/*
 * 0x0d404
 *
 * `fgetc`. Answers the byte, or -1.
 *
 * The fast path is three instructions: if +0 says the buffer still holds
 * something, take a byte and step the pointer at +0xa. Everything else is the
 * slow half.
 *
 * A negative +0, or either of the flags 0x10 and 0x100, or a stream not marked
 * readable at all, is an error at once. Otherwise 0x80 is set - the mark that
 * says this stream has been read from - and the buffer is refilled.
 *
 * After a refill the byte is taken by **jumping back into the fast path**,
 * without re-testing +0. That is safe only because `refill_stream` answers zero
 * exactly when it put something there.
 *
 * A stream with no buffer at all - +6 zero - reads a single byte into a static
 * at DGROUP 0x64c6 instead, and has its own end-of-file dance with `eof`. Not
 * transcribed: every stream the game reads is buffered.
 */
int16_t stdio_fgetc(uint16_t file)
{
    if (file == 0)
        return -1;

    if (DG16(file) <= 0) {
        if (DG16(file) < 0
            || (DGU16(file + 2) & 0x110) != 0
            || (DGU16(file + 2) & 1) == 0) {
            DG16(file + 2) = (int16_t)(DGU16(file + 2) | 0x10);
            return -1;
        }

        DG16(file + 2) = (int16_t)(DGU16(file + 2) | 0x80);

        if (DGU16(file + 6) == 0) {
            not_transcribed("0x0d404's unbuffered path - no stream here is");
            return -1;
        }

        if (refill_stream(file) != 0)
            return -1;
    }

    {
        uint16_t p = DGU16(file + 0xa);

        DG16(file)--;
        DG16(file + 0xa) = (int16_t)(p + 1);
        return DG8(p);
    }
}

/*
 * 0x0d3ef
 *
 * `getc`. Two instructions and a call: **step +0 up** and hand over to `fgetc`.
 *
 * That increment looks pointless until you see who calls it. `buffered_read`
 * decrements +0 to test whether the buffer still holds anything, and only calls
 * here once it has gone negative; this puts it back before `fgetc` looks. The
 * two routines share one counter and each expects the other's convention.
 */
int16_t stdio_getc(uint16_t file)
{
    DG16(file)++;
    return stdio_fgetc(file);
}

/*
 * 0x0ce92
 *
 * `fflush` on one stream. Answers 0, or -1 for a stream that is not open.
 *
 * A null argument means "every open stream", which is 0x0cf13 - not
 * transcribed, and not reached: nothing here flushes them all.
 *
 * The open test is `+0xe == the stream's own address`, which is Borland's way
 * of marking a `FILE` in the table as live without spending a flag.
 *
 * A negative +0 is a stream with buffered *writes*, and its half of this
 * routine is the one that actually writes anything. The game only reads, that
 * branch is never taken, and it is left as a stub.
 *
 * What remains is bookkeeping. The count at +0 is zeroed, and a stream whose
 * pointer sits at `+5` - the one-byte hold field, so an unbuffered stream - has
 * its pointer reset from +8 as well. The `test +2,8` and the two identical
 * comparisons that follow it are the compiler making one condition out of two.
 */
int16_t flush_stream(uint16_t file)
{
    if (file == 0) {
        not_transcribed("0x0cf13, flushing every open stream");
        return 0;
    }

    if (DGU16(file + 0xe) != file)
        return -1;

    if (DG16(file) < 0) {
        not_transcribed("0x0cedd, flushing a write-buffered stream");
        return 0;
    }

    if ((DGU16(file + 2) & 8) == 0) {
        if (DGU16(file + 0xa) != (uint16_t)(file + 5))
            return 0;
    }

    DG16(file) = 0;

    if (DGU16(file + 0xa) != (uint16_t)(file + 5))
        return 0;

    DG16(file + 0xa) = DG16(file + 8);
    return 0;
}

/*
 * 0x0d26c
 *
 * `fseek`. Answers 0, or -1 if the flush or the seek failed.
 *
 * Seeking forward from the current position has to account for what is already
 * in the buffer, and that is 0x0d20f - the unread count, with the newline
 * translation taken off. It is only wanted when the buffer holds something and
 * the whence is 1, which does not happen on these screens, so it is a stub.
 *
 * Then the stream is put back to a clean state: flags 0x10, 0x20 and 0x180
 * cleared - `and +2,0xfe5f` - the count zeroed and the pointer reset to the
 * buffer's start. The seek itself is the DOS one, and only a -1 from it is a
 * failure.
 */
int16_t stdio_fseek(uint16_t file, uint16_t lo, uint16_t hi, int16_t whence)
{
    if (flush_stream(file) != 0)
        return -1;

    if (whence == 1 && DG16(file) > 0) {
        not_transcribed("0x0d20f, the unread count");
        return -1;
    }

    DG16(file + 2) = (int16_t)(DGU16(file + 2) & 0xfe5f);
    DG16(file) = 0;
    DG16(file + 0xa) = DG16(file + 8);

    if (dos_lseek((int8_t)DG8(file + 4), lo, hi, whence) == -1)
        return -1;

    return 0;
}

/*
 * 0x0c27b
 *
 * `tell` on a DOS handle: `lseek` by zero from the current position, which is
 * the only way to ask DOS where a file is. Four pushes and a call.
 */
int32_t dos_tell(int16_t handle)
{
    return dos_lseek(handle, 0, 0, 1);
}

/*
 * 0x0d20f
 *
 * How many bytes of a stream's buffer have not been handed out yet, so that
 * `ftell` can take them off what DOS reports.
 *
 * The count at +0 is negative while the buffer is being filled and positive
 * while it is being drained, and both are turned into the same magnitude - the
 * negative branch by adding the buffer size at +6 and one, the positive one by
 * `cwd`/`xor`/`sub`, which is how a compiler writes `abs`.
 *
 * A stream in binary mode - flag 0x40 - is done there. A text stream then walks
 * the buffer counting newlines, because each one was two bytes in the file; that
 * is not reached here, every stream the game opens being binary, and it is left
 * as a stub.
 *
 * The original cleans its own argument off the stack - `ret 2` - which is
 * Borland's convention for this helper and not a mistake in the caller.
 */
int16_t unread_count(uint16_t file)
{
    int16_t di;

    if (DG16(file) < 0)
        di = (int16_t)(DGU16(file + 6) + DGU16(file) + 1);
    else
        di = (int16_t)(DG16(file) < 0 ? -DG16(file) : DG16(file));

    if ((DGU16(file + 2) & 0x40) == 0) {
        not_transcribed("0x0d20f's newline scan, for a text stream");
        return 0;
    }

    return di;
}

/*
 * 0x0d2d4
 *
 * `ftell`. DOS is asked where the handle is, and then the buffer is accounted
 * for: bytes read ahead and not yet handed out come **off** the answer, bytes
 * written and not yet flushed go **on** to it, and the sign of the count at +0
 * is what says which.
 *
 * A failed `tell` is passed straight through as -1 without the adjustment.
 */
int32_t stdio_ftell(uint16_t file)
{
    int32_t p = dos_tell((int8_t)DG8(file + 4));

    if (p == -1)
        return p;

    if (DG16(file) < 0)
        return p + unread_count(file);

    return p - unread_count(file);
}
