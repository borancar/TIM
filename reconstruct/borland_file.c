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
#include <stdio.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * 0x0bcbb
 *
 * NOT TRANSCRIBED YET. Borland's `exit`: calls the common teardown at 0x0bc64 with (status, 0, 0).
 * Reached only when the start-up gives up.
 */
void stdio_exit(int16_t status)
{
    (void)status;
    not_transcribed("0x0bcbb");
}

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
 * 0x0c1d6
 *
 * Borland's `_setupio`: make the stream table usable before `main` runs.
 *
 * The startup does not call this directly. It walks the init table between
 * DGROUP 0x4e48 and 0x4e4e - six bytes, so one entry - and calls what it finds
 * there, which is this. The port's own start-up calls it by name instead of
 * dispatching through a guest far pointer, because a table of one is not a
 * table.
 *
 * The stream table is at DGROUP 0x4bc4, sixteen bytes an entry, and `_nfile` at
 * 0x4d04 says how many there are - twenty here. Entries 0 to 4 are the standard
 * streams and are already set up; from 5 up this marks each **free** by putting
 * 0xff in the handle byte at +4 and pointing the stream's buffer field at the
 * stream itself.
 *
 * That 0xff is the whole point, and the port went without it for a while: the
 * open path tests `if ((int8_t)handle < 0)` before it opens anything, so a
 * table left as BSS looks like twenty streams that are already open on handle
 * 0, every `fopen` quietly does nothing, and the game gets as far as printing
 * "Unable to initialize vm." because it could not read its own video driver.
 *
 * The two tails give stdin and stdout a buffer, and drop the 0x200 bit from
 * either when it is not a terminal. `stdio_setvbuf`'s mode is 1 for the first
 * and 2 for the second - line buffered and unbuffered - and only when the bit
 * is still set.
 */
void setup_streams(void)
{
    uint16_t dx;

    for (dx = 5; dx < DGU16(0x4d04); dx++) {
        DG16((uint16_t)(0x4d06 + 2 * dx)) = 0;
        DG8((uint16_t)(0x4bc8 + 16 * dx)) = 0xff;
        DGU16((uint16_t)(0x4bd2 + 16 * dx)) = (uint16_t)(0x4bc4 + 16 * dx);
    }

    if (dos_isatty((int16_t)(int8_t)DG8(0x4bc8)) == 0)
        DGU16(0x4bc6) = (uint16_t)(DGU16(0x4bc6) & 0xfdff);

    stdio_setvbuf(0x4bc4, 0, (int16_t)((DGU16(0x4bc6) & 0x200) ? 1 : 0), 0x200);

    if (dos_isatty((int16_t)(int8_t)DG8(0x4bd8)) == 0)
        DGU16(0x4bd6) = (uint16_t)(DGU16(0x4bd6) & 0xfdff);

    stdio_setvbuf(0x4bd4, 0, (int16_t)((DGU16(0x4bd6) & 0x200) ? 2 : 0), 0x200);
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
 * 0x0d754
 *
 * NOT TRANSCRIBED YET. Borland's `printf`: it pushes the formatter at 0x0d8ca,
 * the FILE at DGROUP 0x4bd4 (stdout) and a pointer to its own varargs, and
 * calls the core at 0x0c2ed. The game reaches it only on the two fatal
 * start-up paths, and with a plain string and no arguments both times.
 *
 * It still **aborts**, because the formatting is not reconstructed and a stub
 * that returned quietly would turn a refusal to start into a silent one. But it
 * writes the string out first: the whole value of these two calls is the
 * sentence they print, and losing it would make the abort say much less than
 * the original does.
 */
int16_t stdio_printf(uint16_t fmt)
{
    fputs((const char *)(dgroup + fmt), stderr);
    not_transcribed("0x0d754, printf - the message above is the game's");
    return 0;
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

/*
 * 0x0cd80
 *
 * `_close`: INT 21h AH=3Eh, and clear the handle's entry in the flag table at
 * DGROUP 0x4d06. Answers 0, or -1 through `__IOerror` - which is not
 * transcribed, a close that fails not being something these screens do.
 *
 * The flags are cleared **after** DOS agrees, so a handle DOS refused to close
 * keeps its entry.
 */
int16_t dos_close(int16_t handle)
{
    io_dos_close(handle);
    DG16(0x4d06 + 2 * handle) = 0;
    return 0;
}

/*
 * 0x0cd58
 *
 * `close`: the same, with the handle checked against `_nfile` at DGROUP 0x4d04
 * first, and the flag entry cleared **before** the DOS call rather than after.
 * So the two routines disagree about that ordering, and this is the one the
 * runtime uses.
 *
 * An out-of-range handle is errno 6 through `__IOerror`; not transcribed, and
 * measured as never reached.
 */
int16_t close_handle(int16_t handle)
{
    if ((uint16_t)handle >= DGU16(0x4d04)) {
        not_transcribed("__IOerror for a handle above _nfile");
        return -1;
    }

    DG16(0x4d06 + 2 * handle) = 0;
    return dos_close(handle);
}

/*
 * 0x0ce15
 *
 * `fclose`. Answers what the close answered, or -1 for a stream that is not
 * open - the same `+0xe == the stream's own address` test `flush_stream` uses.
 *
 * A stream with a buffer is flushed first if it was being written to, and its
 * buffer freed if flag 4 says the runtime allocated it. Neither the flush nor
 * the temporary-file cleanup at the end is reached here: the game only reads,
 * and none of its streams has a name to unlink.
 *
 * The `FILE` is then wiped - flags, buffer size and count zeroed, the handle
 * set to 0xff - whether or not the close worked.
 */
int16_t stdio_fclose(uint16_t file)
{
    int16_t si = -1;

    if (DGU16(file + 0xe) != file)
        return -1;

    if (DGU16(file + 6) != 0) {
        if (DG16(file) < 0) {
            not_transcribed("0x0ce30, flushing a written stream on close");
            return -1;
        }
        if ((DGU16(file + 2) & 4) != 0)
            heap_free(DGU16(file + 8));
    }

    if ((int8_t)DG8(file + 4) >= 0)
        si = close_handle((int8_t)DG8(file + 4));

    DG16(file + 2) = 0;
    DG16(file + 6) = 0;
    DG16(file) = 0;
    DG8(file + 4) = 0xff;

    if (DGU16(file + 0xc) != 0) {
        not_transcribed("0x0ce76, unlinking a temporary file on close");
        return -1;
    }

    return si;
}

/*
 * 0x0c018
 *
 * `isatty`: INT 21h AH=44h AL=0, answering bit 7 of the device word - 0x80 for
 * a character device, 0 for a file. Six instructions, and it does not look at
 * the carry flag at all, so a bad handle answers whatever DX happened to hold.
 */
int16_t dos_isatty(int16_t handle)
{
    return (int16_t)(io_dos_devinfo(handle) & 0x80);
}

/*
 * 0x0c8a3
 *
 * The IOCTL call, INT 21h AH=44h, with the sub-function in AL. It answers DX -
 * the device word - when AL is 0 and AX otherwise, and a failure goes to
 * `__IOerror`, which is not transcribed.
 *
 * Only AL=0 is reached here, so only the device word is modelled; the port's
 * `io_dos_devinfo` answers what the emulator does.
 */
int16_t dos_ioctl(int16_t handle, uint16_t al, uint16_t dx, uint16_t cx)
{
    (void)dx;
    (void)cx;

    if (al != 0) {
        not_transcribed("an IOCTL sub-function other than 0");
        return -1;
    }

    return io_dos_devinfo(handle);
}

/*
 * 0x0cd3d
 *
 * `_chmod`: INT 21h AH=43h, with AL choosing between reading the attributes and
 * writing them. Answers CX on success - the attributes - and -1 through
 * `__IOerror` on failure.
 *
 * The runtime uses it as a **file-exists test**: `open_file` asks for the
 * attributes and only looks at bit 0, the read-only flag, and at whether the
 * call worked at all.
 */
int16_t dos_getattr(uint16_t name, uint16_t al, uint16_t cx)
{
    (void)cx;

    if (al != 0) {
        not_transcribed("0x0cd3d writing a file's attributes");
        return -1;
    }

    return io_dos_getattr((const char *)&DG8(name));
}

/*
 * 0x0d707
 *
 * `_open`: INT 21h AH=3Dh. The access mode in AL is worked out from the flags -
 * 2 for read-write, 1 for write-only, 0 for read - and the sharing bits at 0xf0
 * are passed through beside it.
 *
 * On success the handle's entry in the flag table at DGROUP 0x4d06 is set from
 * the flags with 0x0700 cleared and 0x8000 - "open" - added. A failure goes to
 * `__IOerror`, not transcribed.
 */
int16_t dos_open_named(uint16_t name, uint16_t flags)
{
    int16_t h;
    uint8_t access;

    if ((flags & 2) != 0)
        access = 1;
    else if ((flags & 4) != 0)
        access = 2;
    else
        access = 0;

    access = (uint8_t)(access | (flags & 0xf0));

    if (access != 0) {
        not_transcribed("0x0d707 opening for writing or with sharing bits");
        return -1;
    }

    h = io_dos_open((const char *)&DG8(name));
    if (h < 0)
        return io_error(2);               /* DOS 2: file not found */

    DG16(0x4d06 + 2 * h) = (int16_t)((flags & 0xb8ff) | 0x8000);
    return h;
}

/*
 * 0x0cf4d
 *
 * Parse a mode string into the two words `fopen` needs: the open flags, and a
 * permission word for a file that has to be created. Answers a third value -
 * 1, 2 or 3 for read, write and update, with 0x40 added for binary - or 0 for a
 * mode string that starts with none of `r`, `w` or `a`.
 *
 * The `+` may come before or after the `t`/`b`, which is why the second
 * character is looked at twice. With neither `t` nor `b` the default comes from
 * DGROUP 0x4d2e, the global text/binary setting.
 *
 * It also plants a far pointer at DGROUP 0x4bbc on the way out, which is
 * nothing to do with the mode; it is transcribed because it happens.
 *
 * **The segment half of that pointer is a relocation.** In the recovered image
 * the immediate reads 0x0000, because the image is unrelocated; the loader
 * patches it to the program's own base. Transcribing the 0 as written left
 * DGROUP 0x4bbe zero where the original had 0x0110, which is that base. Any
 * immediate that is a segment has to be worked out from where the program
 * actually is, never read off the bytes.
 *
 * The original cleans its own arguments - `ret 6`.
 */
int16_t parse_open_mode(uint16_t out_perm, uint16_t out_flags, uint16_t mode)
{
    uint16_t perm = 0;
    uint16_t flags;
    int16_t r;
    uint8_t c = DG8(mode);

    mode++;

    if (c == 'r') {
        flags = 1;
        r = 1;
    } else if (c == 'w') {
        flags = 0x302;
        perm = 0x80;
        r = 2;
    } else if (c == 'a') {
        flags = 0x902;
        perm = 0x80;
        r = 2;
    } else {
        return 0;
    }

    c = DG8(mode);
    mode++;

    if (c == '+' || (DG8(mode) == '+' && (c == 't' || c == 'b'))) {
        if (c != '+')
            c = DG8(mode);
        flags = (uint16_t)((flags & 0xfffc) | 4);
        perm = 0x180;
        r = 3;
    }

    if (c == 't') {
        flags |= 0x4000;
    } else if (c == 'b') {
        flags |= 0x8000;
        r |= 0x40;
    } else {
        flags |= (uint16_t)(DGU16(0x4d2e) & 0xc000);
        if ((flags & 0x8000) != 0)
            r |= 0x40;
    }

    DG16(0x4bbe) = (int16_t)(IMAGE_BASE >> 4);
    DG16(0x4bbc) = (int16_t)0xdfb4;

    DG16(out_flags) = (int16_t)flags;
    DG16(out_perm) = (int16_t)perm;

    return r;
}

/*
 * 0x0d5af
 *
 * `open`, over `dos_open_named`. Answers the handle, or a negative.
 *
 * With neither text nor binary asked for, the default at DGROUP 0x4d2e is
 * added. The attributes are then read - the file-exists test - and the file
 * opened.
 *
 * Afterwards the handle's flag entry at DGROUP 0x4d06 is rewritten: the flags
 * with 0x0700 cleared, 0x1000 added when the file was opened for writing, and
 * **0x100 added when the file is not read-only** - which is the one thing the
 * attribute read is for.
 *
 * Four branches are stubs and all four were measured as unreached: creating a
 * file, truncating one, the character-device path, and the append seek. The
 * game opens for reading and nothing else.
 */
int16_t open_file(uint16_t name, uint16_t flags, uint16_t perm)
{
    int16_t attr;
    int16_t h;
    int16_t info;

    (void)perm;

    if ((flags & 0xc000) == 0)
        flags |= (uint16_t)(DGU16(0x4d2e) & 0xc000);

    attr = dos_getattr(name, 0, 0);

    if ((flags & 0x100) != 0) {
        not_transcribed("0x0d5e1, creating a file");
        return -1;
    }

    h = dos_open_named(name, flags);
    if (h >= 0) {
        info = dos_ioctl(h, 0, 0, 0);
        if ((info & 0x80) != 0) {
            not_transcribed("0x0d67f, opening a character device");
            return -1;
        }
        if ((flags & 0x200) != 0) {
            not_transcribed("0x0d6a5, seeking to the end for append");
            return -1;
        }
    }

    if (h < 0)
        return h;

    {
        uint16_t v = (uint16_t)((flags & 0xf8ff)
                                | ((flags & 0x300) ? 0x1000 : 0));

        v |= (uint16_t)((attr & 1) ? 0 : 0x100);
        DG16(0x4d06 + 2 * h) = (int16_t)v;
    }

    return h;
}

/*
 * 0x0db5e
 *
 * `setvbuf`. Answers 0, or -1 for a stream that is not open, a mode above 2 or
 * a size above 0x7fff.
 *
 * Two flags at DGROUP 0x4e3c and 0x4e3e remember that `stdin` and `stdout` -
 * the streams at 0x4bc4 and 0x4bd4 - have had a buffer set, so the runtime does
 * not do it again behind the program's back.
 *
 * Whatever the stream had is undone first: seeked back to nought if anything
 * was buffered, the old buffer freed if flag 4 says the runtime owned it, and
 * the pointer set to the one-byte hold field at +5 so an unbuffered stream is
 * consistent before anything else happens.
 *
 * A caller that asks for buffering without supplying a buffer gets one from the
 * heap, and flag 4 records that. Mode 1 adds flag 8, which is line buffering.
 *
 * The far pointer planted at DGROUP 0x4bb8 has the same relocated segment as
 * the one in `parse_open_mode`.
 */
int16_t stdio_setvbuf(uint16_t file, uint16_t buf, int16_t mode, uint16_t size)
{
    if (DGU16(file + 0xe) != file || mode > 2 || size > 0x7fff)
        return -1;

    if (DG16(0x4e3e) == 0 && file == 0x4bd4)
        DG16(0x4e3e) = 1;
    else if (DG16(0x4e3c) == 0 && file == 0x4bc4)
        DG16(0x4e3c) = 1;

    if (DG16(file) != 0)
        stdio_fseek(file, 0, 0, 1);

    if ((DGU16(file + 2) & 4) != 0)
        heap_free(DGU16(file + 8));

    DG16(file + 2) = (int16_t)(DGU16(file + 2) & 0xfff3);
    DG16(file + 6) = 0;
    DG16(file + 8) = (int16_t)(file + 5);
    DG16(file + 0xa) = (int16_t)(file + 5);

    if (mode == 2 || size == 0)
        return 0;

    DG16(0x4bba) = (int16_t)(IMAGE_BASE >> 4);
    DG16(0x4bb8) = (int16_t)0xdfdc;

    if (buf == 0) {
        buf = heap_malloc(size);
        if (buf == 0)
            return -1;
        DG16(file + 2) = (int16_t)(DGU16(file + 2) | 4);
    }

    DG16(file + 0xa) = (int16_t)buf;
    DG16(file + 8) = (int16_t)buf;
    DG16(file + 6) = (int16_t)size;

    if (mode == 1)
        DG16(file + 2) = (int16_t)(DGU16(file + 2) | 8);

    return 0;
}

/*
 * 0x0d0a3
 *
 * Find a free entry in the `FILE` table at DGROUP 0x4bc4, 0x10 bytes apiece and
 * `_nfile` of them, or 0.
 *
 * Free means a **negative** handle byte at +4, which is the 0xff `fclose`
 * leaves. The loop's exit and its found-test are the same comparison written
 * twice, so a table that is entirely full falls out of the bottom and is tested
 * once more before answering 0.
 */
uint16_t find_free_stream(void)
{
    uint16_t si = 0x4bc4;
    uint16_t end = (uint16_t)(0x4bc4 + (DGU16(0x4d04) << 4));

    while ((int8_t)DG8(si + 4) >= 0) {
        uint16_t prev = si;

        si = (uint16_t)(si + 0x10);
        if (end <= prev)
            break;
    }

    if ((int8_t)DG8(si + 4) >= 0)
        return 0;

    return si;
}

/*
 * 0x0d007
 *
 * The body of `fopen`, over a `FILE` the caller has already found. Answers the
 * `FILE`, or 0.
 *
 * The mode string is parsed, the flags stored at +2, and the file opened -
 * unless the caller had already put a handle in +4, in which case the open is
 * skipped and only the flags are taken. Either way a negative handle wipes the
 * `FILE` and answers 0.
 *
 * `isatty` on the handle adds flag 0x200, and that flag then chooses the
 * buffering: a character device gets mode 1, line buffering, and a file gets
 * mode 0 with a 0x200-byte buffer from the heap. A `setvbuf` that fails closes
 * the stream again.
 *
 * The arguments are **mode before name**, which is the opposite of `fopen`'s
 * own order: 0x0d0ce pushes them the other way round. Reading them the way the
 * caller declares them gave a mode string that started with none of `r`, `w`
 * or `a`, so the parse refused and the open never happened.
 *
 * The original cleans its own arguments - `ret 8`.
 */
uint16_t stdio_fopen_into(uint16_t extra_flags, uint16_t mode, uint16_t name,
                          uint16_t file)
{
    uint16_t fp = dg_enter(4);
    uint16_t perm = fp;                    /* [bp-4] */
    uint16_t flags = (uint16_t)(fp + 2);   /* [bp-2] */
    uint16_t r = 0;

    dg_call(8);                            /* three arguments, callee-cleaned */
    DG16(file + 2) = parse_open_mode(perm, flags, mode);
    dg_uncall(8);

    if (DGU16(file + 2) == 0)
        goto fail;

    if ((int8_t)DG8(file + 4) < 0) {
        DG8(file + 4) = (uint8_t)open_file(name,
                                           (uint16_t)(DGU16(flags)
                                                      | extra_flags),
                                           DGU16(perm));
        if ((int8_t)DG8(file + 4) < 0)
            goto fail;
    }

    if (dos_isatty((int8_t)DG8(file + 4)) != 0)
        DG16(file + 2) = (int16_t)(DGU16(file + 2) | 0x200);

    if (stdio_setvbuf(file, 0,
                      (int16_t)((DGU16(file + 2) & 0x200) ? 1 : 0),
                      0x200) != 0) {
        stdio_fclose(file);
        goto fail;
    }

    DG16(file + 0xc) = 0;
    r = file;
    goto out;

fail:
    DG8(file + 4) = 0xff;
    DG16(file + 2) = 0;

out:
    dg_leave(4);
    return r;
}

/*
 * 0x0d0ce
 *
 * `fopen`. Finds a free `FILE` and hands it to the body above with no extra
 * flags. Answers the `FILE`, or 0 when the table is full.
 */
uint16_t stdio_fopen(uint16_t name, uint16_t mode)
{
    uint16_t file = find_free_stream();

    if (file == 0)
        return 0;

    return stdio_fopen_into(0, mode, name, file);
}

/*
 * 0x0be3e
 *
 * A 32-bit shift left, `DX:AX` by `CL`. Two paths - under sixteen bits it
 * shifts both halves and carries the bits that fall off the low one into the
 * high one, sixteen or more it moves the low half up and zeroes it - and the
 * port writes the shift they both compute.
 *
 * The entry is the near door of the pair: `pop bx / push cs / push bx` turns
 * the caller's return address into the far one the `retf` wants.
 *
 * **0x0be41 is the same routine.** These three bytes are the near door - `pop
 * es / push cs / push es`, turning a near caller's return address into the far
 * one the `retf` wants - and a far caller jumps straight past them. The pair at
 * 0x0be7f and 0x0be82 is arranged the same way.
 */
uint32_t long_shift_left(uint32_t v, uint8_t count)
{
    return (uint32_t)(v << count);
}

/*
 * 0x0dd55
 *
 * `stricmp`. Answers the difference of the first pair of characters that
 * differ, with both folded to upper case only **after** they have failed to
 * match exactly - so an exact match never pays for the folding.
 *
 * The letter range is kept in one register pair, `CH` holding `a` and `CL`
 * holding `z`, which is why the two range tests are against a register rather
 * than an immediate.
 *
 * A NUL in the first string ends it before the comparison, so the answer there
 * is `0 - *b`.
 */
int16_t string_compare_nocase(uint16_t a, uint16_t b)
{
    for (;;) {
        uint8_t al = DG8(a);
        uint8_t bl = DG8(b);

        a++;
        if (al == 0)
            return (int16_t)((uint16_t)al - (uint16_t)bl);

        b++;
        if (al == bl)
            continue;

        if (al >= 'a' && al <= 'z')
            al = (uint8_t)(al - 0x20);
        if (bl >= 'a' && bl <= 'z')
            bl = (uint8_t)(bl - 0x20);

        if (al != bl)
            return (int16_t)((uint16_t)al - (uint16_t)bl);
    }
}

/*
 * 0x0ddaf
 *
 * `strncpy`. Copies up to `n` bytes and pads the rest with zeros, which is what
 * the second `rep` is for. Answers the destination.
 *
 * The length is found first with a bounded `repne scasb`, so a source with no
 * NUL inside `n` copies exactly `n` bytes and pads nothing.
 */
uint16_t string_copy_padded(uint16_t dst, uint16_t src, uint16_t n)
{
    uint16_t i = 0;

    while (i < n && DG8((uint16_t)(src + i)) != 0) {
        DG8((uint16_t)(dst + i)) = DG8((uint16_t)(src + i));
        i++;
    }

    if (i < n) {
        DG8((uint16_t)(dst + i)) = 0;
        i++;
    }

    while (i < n) {
        DG8((uint16_t)(dst + i)) = 0;
        i++;
    }

    return dst;
}

/*
 * 0x0bd70
 *
 * `getvect`: INT 21h AH=35h, answering the interrupt vector as `DX:BX` - which
 * the caller reads as `DX:AX` after the `xchg`.
 *
 * The port reads the vector table itself. It is at absolute 0 and is part of
 * the memory the verifier seeds and compares, so this needs nothing invented.
 */
uint32_t dos_getvect(uint16_t n)
{
    const uint8_t *v = guest_mem + 4 * (n & 0xff);

    return ((uint32_t)*(const uint16_t *)(v + 2) << 16)
           | *(const uint16_t *)v;
}

/*
 * 0x0bd7f
 *
 * `setvect`: INT 21h AH=25h. Ten instructions, of which two are saving and
 * restoring DS around the `lds` that loads the handler.
 *
 * The port writes the vector table directly, for the same reason `getvect`
 * reads it.
 */
void dos_setvect(uint16_t n, uint16_t off, uint16_t seg)
{
    uint8_t *v = guest_mem + 4 * (n & 0xff);

    *(uint16_t *)v = off;
    *(uint16_t *)(v + 2) = seg;
}

/*
 * 0x0bfcd
 *
 * `__IOerror`: turn a DOS error code into `errno`, and answer -1 so the caller
 * can `return __IOerror(ax)`.
 *
 * A **positive** code is a DOS one: it is remembered at DGROUP 0x4d34 as
 * `_doserrno` and mapped through the table at 0x4d36 to a C errno. Anything
 * above 0x58 is clamped to 0x57 first, so an unknown code lands on the last
 * entry rather than off the end of the table.
 *
 * A **negative** code is already a C errno, negated: `_doserrno` is set to -1
 * and the value used directly. That path also clamps, by jumping into the
 * positive path's clamp - so a negated value beyond 0x23 is turned into DOS
 * code 0x57 and mapped, which is not what the negation meant. Transcribed as it
 * stands.
 *
 * `errno` is DGROUP 0x94. The original cleans its own argument - `ret 2`.
 */
int16_t io_error(int16_t code)
{
    int16_t si = code;

    if (si >= 0) {
        if (si > 0x58)
            si = 0x57;
        DG16(0x4d34) = si;
        si = (int16_t)(int8_t)DG8((uint16_t)(si + 0x4d36));
    } else {
        si = (int16_t)(-si);
        if (si > 0x23) {
            si = 0x57;
            DG16(0x4d34) = si;
            si = (int16_t)(int8_t)DG8((uint16_t)(si + 0x4d36));
        } else {
            DG16(0x4d34) = -1;
        }
    }

    DG16(0x94) = si;
    return -1;
}

/*
 * 0x0dd33
 *
 * `strcpy`. The length is found first with a `repne scasb` over 0xffff bytes
 * and the copy is one `rep movsb`, so the NUL is copied with the rest. Answers
 * the destination.
 */
uint16_t string_copy(uint16_t dst, uint16_t src)
{
    uint16_t i = 0;

    for (;;) {
        DG8((uint16_t)(dst + i)) = DG8((uint16_t)(src + i));
        if (DG8((uint16_t)(src + i)) == 0)
            break;
        i++;
    }

    return dst;
}

/*
 * 0x0bb4f
 *
 * The far-callable face of `strcpy`: it takes the two words off the stack and
 * hands them straight on.
 */
uint16_t string_copy_far(uint16_t dst, uint16_t src)
{
    return string_copy(dst, src);
}

/*
 * 0x0bd4a
 *
 * `getdate`: INT 21h AH=2Ah, with the year written to the caller's +0 and the
 * packed month and day to +2. The weekday DOS puts in AL is dropped.
 */
void dos_getdate(uint16_t out)
{
    uint16_t year, monthday, weekday;

    io_dos_getdate(&year, &monthday, &weekday);

    DG16(out) = (int16_t)year;
    DG16(out + 2) = (int16_t)monthday;
}

/*
 * 0x0dc95
 *
 * `strcat`. Both strings are measured with a `repne scasb` first, and the copy
 * is words with a trailing byte.
 *
 * The alignment step - `test si,1`, then `movsb` and **`dec cx`** - takes its
 * byte off the count, so the total is right either way. That `dec` is easy to
 * miss: it is the single byte between the `movsb` and the `shr`, and a
 * disassembly window that stops at the `movsb` does not show it, and reading it
 * as absent turns a correct routine into an apparent off-by-one.
 *
 * `far_memcpy` at 0x222c6 has the same `movsb`/`dec cx` pair guarded by `jae`
 * instead of `je` - and `test` always clears carry, so there the alignment step
 * never runs at all. The count stays right either way; only the alignment is
 * lost. The two routines are wrong and right in different places.
 */
uint16_t string_concat(uint16_t dst, uint16_t src)
{
    uint16_t d = dst;
    uint16_t n = 0;

    while (DG8(d) != 0)
        d++;

    while (DG8((uint16_t)(src + n)) != 0)
        n++;
    n++;                                  /* the NUL counts */

    if ((src & 1) != 0) {
        DG8(d) = DG8(src);
        d++;
        src++;
        n--;
    }

    while (n-- != 0) {
        DG8(d) = DG8(src);
        d++;
        src++;
    }

    return dst;
}

/*
 * 0x0c1b2
 *
 * `setbuf`: `setvbuf` with a fixed size of 0x200 and the mode chosen by whether
 * a buffer was given - 0 for full buffering with one, 2 for none without.
 */
int16_t stdio_setbuf(uint16_t file, uint16_t buf)
{
    return stdio_setvbuf(file, buf, (int16_t)(buf != 0 ? 0 : 2), 0x200);
}

/*
 * 0x094fb
 *
 * **`fwrite`, through the archive layer.** A pointer, an element size, a count
 * and a file, answering how many elements went - and every caller in the
 * machine writer compares that with 1.
 *
 * **A file may be an entry in the resource archive rather than a file of its
 * own**, and when the archive is in use at DGROUP 0x547e this asks
 * `archive_entry_for` first. An entry writes to the handle at its +0x10; an
 * entry without one writes *nothing* and answers zero, which the callers then
 * read as a short write. A file that is not an entry at all falls through to
 * the plain path, and so does everything when the archive is not in use.
 *
 * The failure mark at DGROUP 0x567b is **or-ed, not set**: it accumulates
 * across every write anyone does rather than describing this one. That is a
 * different thing from the machine writer's own 0x5478, which is per-file and
 * checked before each field - the two exist together and neither is the other.
 */
uint16_t game_fwrite(uint16_t ptr, uint16_t size, uint16_t count,
                     uint16_t file)
{
    uint16_t n;

    if (DGU16(0x547e) != 0) {
        uint16_t entry = archive_entry_for(file);

        if (entry != 0) {
            if (DGU16((uint16_t)(entry + 0x10)) != 0)
                n = sub_0d321(ptr, size, count,
                              DGU16((uint16_t)(entry + 0x10)));
            else
                n = 0;

            DGU16(0x567b) |= (uint16_t)(n != count ? 1 : 0);
            return n;
        }
    }

    n = sub_0d321(ptr, size, count, file);

    DGU16(0x567b) |= (uint16_t)(n != count ? 1 : 0);
    return n;
}

/*
 * 0x0d321
 *
 * **`fwrite`'s body**: turn elements into bytes, write them, and turn the bytes
 * back into elements.
 *
 * A size of zero answers the *count* rather than zero, which is the C library's
 * answer to "write `count` things of no length" - it wrote all of them, and
 * none of them took any room. Answering zero there would look like failure to
 * every caller.
 *
 * The product is worked out as a **long**, and anything that does not fit a word
 * answers zero without writing: this cannot write 64 KB or more in one call. The
 * test is `dx > 1` then `dx < 1`, and the third branch - `or ax, ax` followed by
 * `jae` - can only go one way, because `or` clears the carry. So a high word of
 * exactly 1 also answers zero, and the instruction that looks like it is
 * deciding something is a comparison the compiler left behind.
 *
 * The answer is the bytes written divided by the size, so a partial write of the
 * last element is not counted - the caller learns that fewer elements went, not
 * that some fraction did.
 */
uint16_t sub_0d321(uint16_t ptr, uint16_t size, uint16_t count,
                   uint16_t file)
{
    uint32_t total;

    if (size == 0)
        return count;

    total = (uint32_t)size * (uint32_t)count;
    if (total > 0xFFFF)
        return 0;

    return (uint16_t)(sub_0d8ca(file, (uint16_t)total, ptr) / size);
}

/*
 * 0x0d8ca
 *
 * NOT TRANSCRIBED YET. Put a run of bytes on a file - handle, length, pointer -
 * answering how many went. It is what `fwrite` narrows to, and Borland's
 * `printf` hands its address to the formatter as the sink to write through, so
 * the two reach the file the same way.
 */
uint16_t sub_0d8ca(uint16_t file, uint16_t len, uint16_t ptr)
{
    (void)file;
    (void)len;
    (void)ptr;
    not_transcribed("0x0d8ca");
    return 0;
}

/*
 * 0x0b794
 *
 * NOT TRANSCRIBED YET. Borland's `unlink`: INT 21h AH=41h with the path in DX,
 * answering 0 on success and the DOS error otherwise, filed at DGROUP 0x2d7b
 * like the rest. The machine writer uses it to delete a file it failed to
 * finish, so a half-written machine cannot be loaded.
 *
 * A stub for the same reason as `dos_chdir` below: the port's files come from
 * one fixed directory it opens read-only, and there is nothing here that may
 * delete one.
 */
uint16_t dos_unlink(uint16_t path)
{
    (void)path;
    not_transcribed("0x0b794");
    return 0;
}

/*
 * 0x0b755
 *
 * NOT TRANSCRIBED YET. Borland's `chdir`: INT 21h AH=3Bh with the path in DX,
 * answering 0 on success and the DOS error code otherwise, and filing that
 * same value at DGROUP 0x2d7b - which is `errno`.
 *
 * Left as a stub because the port has nowhere to change to: it reaches the
 * game's files through `io_dos_*` from one fixed directory, read-only. Making
 * this answer 0 would be inventing a success the port cannot honour, and the
 * caller uses the answer to decide whether to select a drive.
 */
uint16_t dos_chdir(uint16_t path)
{
    (void)path;
    not_transcribed("0x0b755");
    return 0;
}

/*
 * 0x0b819
 *
 * NOT TRANSCRIBED YET. Borland's `setdisk`: INT 21h AH=0Eh, with the drive
 * taken from a *letter* - `and al, 0x5f` uppercases it and `sub al, 0x41`
 * makes it the number DOS wants, so 'a' and 'A' are both drive zero.
 *
 * A stub for the same reason as `dos_chdir` above.
 */
void dos_setdisk(uint16_t letter)
{
    (void)letter;
    not_transcribed("0x0b819");
}

/*
 * 0x0b7b3
 *
 * `getcurdir`-style: write the current drive and directory into the caller's
 * buffer as `X:\\` followed by the path.
 *
 * The drive letter comes from INT 21h AH=19h plus 0x41, so drive 0 is `A`. The
 * path is then asked for with AH=47h **for drive 0** - `dl` is zeroed, which
 * DOS reads as "the current drive" - and written straight after the backslash,
 * without its own leading one, which is why the backslash is put there first.
 *
 * Nothing checks whether either call failed.
 */
void dos_get_cur_dir(uint16_t buf)
{
    DG8(buf) = (uint8_t)(io_dos_curdrive() + 0x41);
    DG8(buf + 1) = ':';
    DG8(buf + 2) = '\\';

    /*
         * The cast goes through `uintptr_t` because DGROUP is volatile - the
         * timer handler runs on a thread and shares it - and this one place
         * hands a pointer *into* it to the IO layer, which fills the buffer
         * itself. Nothing else does that, and the volatility is the port's
         * memory model rather than anything the original had.
         */
        io_dos_getcwd((uint8_t *)(uintptr_t)(dgroup + (uint16_t)(buf + 3)));
}
