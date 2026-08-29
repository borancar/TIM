
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x1c250..0x248f0 - the largest of the game's own modules, 112 call targets.
 * Functions are in address order and each carries the image offset it was read
 * from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * NOT a transcription: the parity flag, worked out in C. The original gets it
 * from `or di,di` for free; see its one caller, `far_memset`, for why it is
 * being used at all. It is hoisted here rather than left beside that caller
 * because this file is in address order and the caller is not the first
 * routine in it.
 */
static int32_t low_byte_parity_even(uint16_t v)
{
    /* PF is set when the low eight bits hold an even number of set bits. */
    uint8_t b = (uint8_t)(v & 0xFF);
    int32_t n = 0;

    while (b) {
        n ^= 1;
        b = (uint8_t)(b & (b - 1));
    }
    return n == 0;
}
/*
 * 0x1c278
 *
 * Decompression type 1: plain run-length coding, and the first of the three
 * handlers the table at DGROUP 0x3580 dispatches to.
 *
 * Each token is one byte. Bit 7 clear means the low seven bits are a count of
 * literal bytes to copy; bit 7 set means they are a count and the **next** byte
 * is the value to repeat. A token of -1 - the end of the input - stops it, and
 * so does either emitter answering 0, which is how the output side says the
 * caller's request has been filled.
 *
 * Bit 0x20 at DGROUP 0x57ba selects a different routine entirely at 0x1cd2c.
 * That is not reached on these screens and is left as a stub.
 *
 * The answer is always 0 on the run-length path: nothing here reports how much
 * it produced, because 0x1c92b works that out from what is left at 0x5890.
 */
int16_t decompress_rle(void)
{
    int16_t di = 1;

    if ((DG8(0x57ba) & 0x20) == 0) {
        not_transcribed("0x1cd2c, the other type-1 path");
        return 0;
    }

    while (di != 0) {
        int16_t si = next_input_byte();

        if (si == -1)
            break;

        if ((si & 0x80) != 0)
            di = emit_fill_run((uint16_t)next_input_byte(),
                               (uint16_t)(si & 0x7f));
        else
            di = emit_literal_run((uint16_t)(si & 0x7f));
    }

    return 0;
}

/*
 * 0x1c319
 *
 * Copy `count` bytes out of the current resource into a huge pointer, through a
 * 0x32-byte staging buffer at DGROUP 0x5788.
 *
 * The buffer is why this is a loop at all: `game_fread` reads into DGROUP, and
 * the destination is a huge pointer that may be anywhere, so each pass reads at
 * most 0x32 bytes and then `far_memcpy`s them out.
 *
 * The destination is advanced by `huge_add_to` on **its own argument slot** -
 * `lea ax,[bp+4]` - so the far pointer the caller passed by value is stepped in
 * place and stays normalised. The port reserves the slot on the guest stack
 * because that address has to be a real DGROUP address; see `dg_enter`.
 *
 * The loop ends on a short read as well as on the count running out, and the
 * answer is 0 either way: nothing here reports how much it managed.
 */
int16_t read_into_huge(uint16_t dst_off, uint16_t dst_seg, uint16_t count)
{
    uint16_t fp = dg_enter(4);
    int16_t si = (int16_t)count;
    int16_t di = 1;

    DG16(fp) = (int16_t)dst_off;
    DG16(fp + 2) = (int16_t)dst_seg;

    while (si != 0 && di > 0) {
        uint16_t n = (uint16_t)(si > 0x32 ? 0x32 : si);

        di = (int16_t)game_fread(0x5788, 1, n, DGU16(0x57bc));
        si = (int16_t)(si - di);

        far_memcpy(DGU16(fp), DGU16(fp + 2), 0x5788, DGROUP_SEG,
                   (uint16_t)di);

        huge_add_to(fp, DGROUP_SEG, (int32_t)di);
    }

    dg_leave(4);
    return 0;
}

/*
 * 0x1c3e6
 *
 * Read up to `count` bytes of the compressed stream into DGROUP, and answer how
 * many. This is what fills the bit buffer the LZW code reader works out of.
 *
 * How much is left is `+0xe:+0x10` minus `+0xa:+0xc` on the record at DGROUP
 * 0x588a, and the request is cut down to it - a 32-bit comparison that is
 * signed on the high half and unsigned on the low, which is the compiler
 * comparing a `long` against a zero-extended `int`.
 *
 * The position advances by what will be taken **before** anything is taken, and
 * then the same bit 0x20 at DGROUP 0x5888 that `next_input_byte` uses chooses
 * between the file and a block already in memory - there through
 * `huge_add_to` on the cursor at 0x5898.
 */
int16_t read_input_block(uint16_t dst, uint16_t count)
{
    uint16_t rec = DGU16(0x588a);
    uint16_t rem_lo = (uint16_t)(DGU16(rec + 0xe) - DGU16(rec + 0xa));
    uint16_t rem_hi = (uint16_t)(DGU16(rec + 0x10) - DGU16(rec + 0xc)
                                 - (DGU16(rec + 0xe) < DGU16(rec + 0xa)
                                    ? 1 : 0));
    uint16_t n_lo, n_hi;

    if (rem_lo == 0 && rem_hi == 0)
        return 0;

    if ((int16_t)rem_hi > 0 || (rem_hi == 0 && count > rem_lo)) {
        n_hi = rem_hi;
        n_lo = rem_lo;
    } else {
        n_hi = 0;
        n_lo = count;
    }

    DG16(rec + 0xa) = (int16_t)(DGU16(rec + 0xa) + n_lo);
    DG16(rec + 0xc) = (int16_t)(DGU16(rec + 0xc) + n_hi
                                + (DGU16(rec + 0xa) < n_lo ? 1 : 0));

    if ((DG8(0x5888) & 0x20) != 0)
        return (int16_t)game_fread(dst, 1, n_lo, DGU16(0x57bc));

    far_memcpy(dst, DGROUP_SEG, DGU16(0x5898), DGU16(0x589a), n_lo);
    huge_add_to(0x5898, DGROUP_SEG,
                (int32_t)(((uint32_t)n_hi << 16) | n_lo));

    return (int16_t)n_lo;
}

/*
 * 0x1c493
 *
 * Deliver a run of `n` literal bytes to the output.
 *
 * The output has two states and DGROUP 0x5890 - what the caller of 0x1c92b
 * still wants - decides between them. While the run fits, the bytes go to the
 * destination huge pointer at 0x5894, which is then stepped by `huge_add_to`,
 * and the answer is 1 meaning "keep going". Once it does not fit they spill
 * into the small buffer at 0x5892 with a count at the record's +0x1a, and the
 * answer is 0.
 *
 * The input position at the record's +0xa:+0xc advances by `n` **before**
 * either, so it counts what was consumed rather than what was delivered.
 *
 * Bit 0x40 at DGROUP 0x57ba is what makes the write happen at all; without it
 * the bytes are skipped in the file instead, by seeking forward over them. That
 * branch is not reached on these screens.
 *
 * The spill writes at the **start** of the buffer, not at the count it has just
 * increased - unlike 0x1c51e, which offsets by the old count. Transcribed as it
 * stands; nothing reaches it here either.
 */
int16_t emit_literal_run(uint16_t n)
{
    uint16_t rec = DGU16(0x588a);

    DG16(rec + 0xa) = (int16_t)(DGU16(rec + 0xa) + n);
    if (DGU16(rec + 0xa) < n)
        DG16(rec + 0xc) = (int16_t)(DGU16(rec + 0xc) + 1);

    if (DGU16(0x5890) < n) {
        rec = DGU16(0x588a);
        DG8(rec + 0x1a) = (uint8_t)(DG8(rec + 0x1a) + n);
        read_into_huge(DGU16(0x5892), DGROUP_SEG, n);
        return 0;
    }

    if ((DG8(0x57ba) & 0x40) != 0)
        read_into_huge(DGU16(0x5894), DGU16(0x5896), n);
    else
        game_fseek(DGU16(0x57bc), n, 0, 1);

    DG16(0x5890) = (int16_t)(DGU16(0x5890) - n);
    huge_add_to(0x5894, DGROUP_SEG, (int32_t)n);

    return 1;
}

/*
 * 0x1c51e
 *
 * Deliver a run of `n` copies of one byte - the other half of the run-length
 * pair, and the same two output states as `emit_literal_run`, reached the same
 * way and answering the same 1 or 0.
 *
 * Nothing is read here, so nothing advances the input position: that was done
 * by the two `next_input_byte` calls the caller made to get the length and the
 * value.
 *
 * The spill offsets by the record's +0x1a **before** adding to it, which is
 * what 0x1c493's spill does not do. Neither is reached on these screens.
 */
int16_t emit_fill_run(uint16_t value, uint16_t n)
{
    uint16_t rec;

    if (DGU16(0x5890) < n) {
        rec = DGU16(0x588a);
        far_memset((uint16_t)(DGU16(0x5892) + DG8(rec + 0x1a)), DGROUP_SEG,
                   value, n, (uint16_t)((int16_t)n < 0 ? 0xffff : 0));
        rec = DGU16(0x588a);
        DG8(rec + 0x1a) = (uint8_t)(DG8(rec + 0x1a) + n);
        return 0;
    }

    if ((DG8(0x57ba) & 0x40) != 0)
        far_memset(DGU16(0x5894), DGU16(0x5896), value,
                   n, (uint16_t)((int16_t)n < 0 ? 0xffff : 0));

    DG16(0x5890) = (int16_t)(DGU16(0x5890) - n);
    huge_add_to(0x5894, DGROUP_SEG, (int32_t)(int16_t)n);

    return 1;
}

/*
 * 0x1c5a3
 *
 * Deliver one byte - `emit_literal_run` and `emit_fill_run` written for a run
 * of exactly one, with the same two states and the same answers.
 *
 * The spill half is reached here, unlike in the other two, and it reads the
 * record's +0x1a and increments it in one instruction - `mov al,[bx+0x1a]` then
 * `inc byte [bx+0x1a]` - so the byte lands at the old count.
 */
int16_t emit_byte(uint16_t value)
{
    if (DGU16(0x5890) >= 1) {
        if ((DG8(0x57ba) & 0x40) != 0)
            *FAR_PTR(DGU16(0x5896), DGU16(0x5894)) = (uint8_t)value;

        huge_add_to(0x5894, DGROUP_SEG, 1);
        DG16(0x5890) = (int16_t)(DGU16(0x5890) - 1);
        return 1;
    }

    {
        uint16_t rec = DGU16(0x588a);
        uint8_t n = DG8(rec + 0x1a);

        DG8(rec + 0x1a) = (uint8_t)(n + 1);
        DG8((uint16_t)(DGU16(0x5892) + n)) = (uint8_t)value;
        return 0;
    }
}

/*
 * 0x1ca62
 *
 * Decompression type 2: LZW, hand-written assembly, and the only routine here
 * that has to be able to **stop in the middle** and be called again.
 *
 * The dictionary is the block at DGROUP 0x588c:0x588e - prefix codes as words
 * from offset 0, suffix bytes from 0x2720 - and 0x372 paragraphs above it, at
 * offset 0x3720, is a scratch area the decoded string is built in.
 *
 * A code is expanded by walking the prefix chain, which produces the string
 * **backwards**, so it is written forward into scratch and then copied out
 * backwards. That is what the `sub si,2` after each `lodsb` is doing: one
 * forward from the load, two back, net one back.
 *
 * A code at or above the next free one is the case where the string being
 * decoded is the one about to be defined; the last byte emitted goes down
 * first and the previous code is expanded behind it.
 *
 * Code 0x100 clears the dictionary - 0x200 bytes filled with the *offset* of
 * the dictionary, which is zero in practice but transcribed as written - and
 * arms the reset flag `next_lzw_code` reads.
 *
 * Both copy loops are unrolled ten times in the original and are written here
 * as the loops they are; nothing depends on the unrolling.
 *
 * The suspend at 0x1cbf9 is the interesting part. When the caller's request
 * fills up mid-string, the scratch position is parked at DGROUP 0x35d1, the
 * byte that did not fit is spilled into the small buffer at 0x5892, and 0x58a2
 * is set. The next call comes back in at the middle of whichever copy loop it
 * left, with the destination, the count and the scratch position restored -
 * which is why this is not written as a plain loop over codes.
 *
 * Answers 1 when it suspended, and whatever `next_lzw_code` answered - a
 * negative - at the end of the input.
 */
int16_t decompress_lzw(void)
{
    uint16_t scratch_seg = (uint16_t)(DGU16(0x588e) + 0x372);
    uint16_t dst_off, dst_seg;
    uint16_t si, di, cx;
    int16_t code;
    uint8_t al = 0;
    int16_t copying;

    if (DG8(0x58a2) != 0) {
        cx = (uint16_t)(DGU16(0x5890) + 1);
        dst_off = DGU16(0x5894);
        dst_seg = DGU16(0x5896);
        si = DGU16(0x35d1);
        copying = (DG8(0x57ba) & 0x40) != 0;
        DG8(0x58a2) = 0;
        di = dst_off;
        goto step_back;
    }

    if (DG8(0x58ae) != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        DG8(0x58ae) = 0;
        code = next_lzw_code();
        DG16(0x58a6) = code;
        DG16(0x58ac) = code;
        emit_byte((uint16_t)code);
    }

    for (;;) {
        code = next_lzw_code();
        if (code < 0)
            return code;

        if (code == 0x100) {
            uint16_t p = DGU16(0x588c);
            int16_t i;

            for (i = 0; i < 0x100; i++)
                *(uint16_t *)FAR_PTR(DGU16(0x588e),
                                     (uint16_t)(p + 2 * i)) = p;

            DG16(0x58a4) = (int16_t)(p + 1);
            DG16(0x58a0) = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        di = 0;
        si = (uint16_t)code;
        DG16(0x58b0) = code;

        if ((int16_t)si >= DG16(0x58a0)) {
            *FAR_PTR(scratch_seg, di++) = (uint8_t)DGU16(0x58ac);
            si = DGU16(0x58a6);
        }

        while (si >= 0x100) {
            *FAR_PTR(scratch_seg, di++) =
                *FAR_PTR(DGU16(0x588e), (uint16_t)(0x2720 + si));
            si = *(uint16_t *)FAR_PTR(DGU16(0x588e), (uint16_t)(si << 1));
        }

        al = *FAR_PTR(DGU16(0x588e), (uint16_t)(0x2720 + si));
        *FAR_PTR(scratch_seg, di++) = al;
        DG16(0x58ac) = al;

        cx = (uint16_t)(DGU16(0x5890) + 1);
        si = (uint16_t)(di - 1);
        dst_off = DGU16(0x5894);
        dst_seg = DGU16(0x5896);
        di = dst_off;
        copying = (DG8(0x57ba) & 0x40) != 0;

        for (;;) {
            al = *FAR_PTR(scratch_seg, si);
            si++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                uint16_t rec;

                DG16(0x5894) = (int16_t)di;
                DG16(0x35d1) = (int16_t)si;

                rec = DGU16(0x588a);
                {
                    uint16_t n = DGU16(rec + 0x1a) & 0xff;

                    DG16(rec + 0x1a) = (int16_t)(DGU16(rec + 0x1a) + 1);
                    DG8((uint16_t)(DGU16(0x5892) + n)) = al;
                }

                DG16(0x5890) = 0;
                DG8(0x58a2) = 1;
                return 1;
            }

            if (copying)
                *FAR_PTR(dst_seg, di) = al;
            di++;

step_back:
            si = (uint16_t)(si - 2);
            if ((int16_t)si < 0)
                break;
        }

        /* 0x1cc22 - this code is done and the dictionary can grow. */
        cx--;
        DG16(0x5890) = (int16_t)cx;
        DG16(0x5894) = (int16_t)di;

        if (DG16(0x58a0) < 0x1000) {
            uint16_t next = DGU16(0x58a0);

            *(uint16_t *)FAR_PTR(DGU16(0x588e), (uint16_t)(next << 1)) =
                DGU16(0x58a6);
            DG16(0x58a0) = (int16_t)(next + 1);
            *FAR_PTR(DGU16(0x588e), (uint16_t)(next + 1 + 0x271f)) =
                (uint8_t)DGU16(0x58ac);
        }

        DG16(0x58a6) = DG16(0x58b0);
    }
}

/*
 * 0x1c92b
 *
 * Deliver the next `count` bytes of a resource, decompressing as needed, and
 * answer how many were actually delivered.
 *
 * Everything the three decompressors do is bookkeeping around DGROUP 0x5890,
 * which starts as what was asked for and is counted down as bytes are
 * produced; what came out is the difference. `resource_advance` runs first to
 * hand over anything left over from the last call, and again afterwards if the
 * request is still not full.
 *
 * The handler is chosen by the byte at DGROUP 0x57be, indexing a table at
 * DGROUP 0x3580 **fourteen bytes to the entry** with the near offset first.
 * Which entries are live was measured by hooking the indirect call, not read
 * off the table: exactly three, and the port maps their offsets back to the
 * routines rather than pretending to know the whole table.
 *
 * The two words at the record's +0x16:+0x18 are a running total of everything
 * this resource has produced.
 *
 * The first argument is not read. `resource_advance` takes none, and the handle
 * it would name is reached through a global.
 */
int16_t resource_read(uint16_t handle, uint16_t count)
{
    uint16_t rec;
    int16_t got;

    (void)handle;

    DG16(0x5890) = (int16_t)count;
    resource_advance();

    if (DG16(0x5890) != 0) {
        uint16_t entry = DGU16(0x3580 + 14 * DG8(0x57be));

        switch (entry) {
        case 0x0028:                    /* image 0x1c278 */
            decompress_rle();
            break;
        case 0x0812:                    /* image 0x1ca62 */
            decompress_lzw();
            break;
        case 0x25a2:                    /* image 0x1e7f2 */
            decompress_lzss();
            break;
        default:
            not_transcribed("a handler in the table at DGROUP 0x3580");
            break;
        }

        if (DG16(0x5890) != 0)
            resource_advance();
    }

    got = (int16_t)(count - DGU16(0x5890));

    rec = DGU16(0x588a);
    DG16(rec + 0x16) = (int16_t)(DGU16(rec + 0x16) + got);
    if (DGU16(rec + 0x16) < (uint16_t)got)
        DG16(rec + 0x18) = (int16_t)(DGU16(rec + 0x18) + 1);

    return got;
}

/*
 * 0x1cc65
 *
 * The next LZW code, 9 to 12 bits wide, out of a bit buffer at DGROUP 0x35bc.
 * Answers -1 at the end of the input.
 *
 * Three things can happen before a code is extracted, and they fall through
 * into one another:
 *
 *   the next free code has passed the width's limit at 0x58b6, so the width at
 *   0x589e goes up by one and the limit with it - `1 << width` minus one,
 *   except at twelve bits where it is 0x1000 rather than 0xfff;
 *
 *   0x58a4 says the dictionary is to be reset, so the width goes back to nine
 *   and the limit to 0x1ff;
 *
 *   the bit buffer is empty, so `read_input_block` fills it with `width` bytes.
 *
 * A widening or a reset **always** refills, discarding whatever bits were left.
 * That is not a mistake: the compressor pads to a byte boundary when the width
 * changes, which is what makes the two ends agree.
 *
 * The buffer holds at most twelve bytes, so the bit position is under 108 and
 * the `shr ax,cl` that follows `lodsb` is shifting a byte even though AH still
 * holds the high half of the position `add` - it is zero every time.
 *
 * The mask table at DGROUP 0x35c8 is indexed by how many bits are still wanted.
 */
int16_t next_lzw_code(void)
{
    uint16_t bitpos;
    uint16_t si, ax, dx;
    uint8_t ch, bl;

    if ((int16_t)DGU16(0x58a0) > DG16(0x58b6)) {
        uint16_t cx = (uint16_t)(DGU16(0x589e) + 1);

        DG16(0x589e) = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            DG16(0x58b6) = 0x1000;
        else
            DG16(0x58b6) = (int16_t)((1 << (cx & 0xff)) - 1);

        if (DG16(0x58a4) != 0) {
            DG16(0x589e) = 9;
            DG16(0x58b6) = 0x1ff;
            DG16(0x58a4) = 0;
        }
    } else if (DG16(0x58a4) != 0) {
        DG16(0x589e) = 9;
        DG16(0x58b6) = 0x1ff;
        DG16(0x58a4) = 0;
    } else if (DG16(0x58b2) < DG16(0x58b4)) {
        goto extract;
    }

    {
        uint16_t width = DGU16(0x589e);
        int16_t n = read_input_block(0x35bc, width);

        if (n <= 0) {
            DG16(0x58b4) = n;
            return -1;
        }

        DG16(0x58b2) = 0;
        DG16(0x58b4) = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = DGU16(0x58b2);
    bl = (uint8_t)DGU16(0x589e);
    ch = (uint8_t)bitpos;

    DG16(0x58b2) = (int16_t)(bitpos + DGU16(0x589e));

    si = (uint16_t)(0x35bc + (bitpos >> 3));
    ch &= 7;

    ax = DG8(si);
    si++;
    ax = (uint16_t)(ax >> ch);
    dx = ax;

    ch = (uint8_t)(-(int8_t)(ch - 8));
    bl = (uint8_t)(bl - ch);

    if ((int8_t)bl >= 8) {
        ax = DG8(si);
        si++;
        ax = (uint16_t)(ax << ch);
        dx |= ax;
        ch = (uint8_t)(ch + 8);
        bl = (uint8_t)(bl - 8);
    }

    ax = DG8(0x35c8 + bl);
    ax &= DG8(si);
    ax = (uint16_t)(ax << ch);

    return (int16_t)(ax | dx);
}

/*
 * 0x1c649
 *
 * Select a resource by handle and unpack its entry into the globals the rest of
 * the loader reads. A **near** call, so its argument is at [bp+4].
 *
 * The table at DGROUP 0x57c0 holds 0x64 near pointers, one per handle, and a
 * handle outside 0..0x63 or naming a null entry answers 0. Note the low bound
 * is a *signed* test, so a negative handle is rejected rather than wrapping.
 *
 * The entry's byte at +0x20 is both a flag set and a small number: the whole
 * byte goes to 0x5888, its low five bits to 0x57be, and bit 0x20 selects
 * between two ways of finding the data.
 *
 * With bit 0x20 set the resource is already somewhere known and only its +6 is
 * kept. Without it, the data lies at a 32-bit offset from a far pointer - +6/+8
 * is the base and +0xa/+0xc the offset - and the two are added and normalised.
 * The original does that through the runtime's huge-pointer add at 0x0bf0a,
 * which folds the sum down until the offset is a single nibble; the port
 * computes the same linear address directly, and `normalise_far_ptr_far` then
 * runs over it exactly as the original's does.
 */
int16_t select_resource(int16_t handle)
{
    uint16_t entry;

    if (handle < 0 || handle >= 0x64)
        return 0;

    entry = DGU16(0x57c0 + 2 * handle);
    DG16(0x588a) = (int16_t)entry;
    if (entry == 0)
        return 0;

    DG16(0x588e) = DG16(entry + 4);
    DG16(0x588c) = DG16(entry + 2);
    DG16(0x5892) = DG16(entry);

    DG8(0x5888) = DG8(entry + 0x20);
    DG8(0x57be) = (uint8_t)(DG8(0x5888) & 0x1f);

    if ((DG8(0x5888) & 0x20) != 0) {
        DG16(0x57bc) = DG16(entry + 6);
        DG8(0x57ba) = 0x20;
        return 1;
    }

    DG8(0x57ba) = 0;
    {
        uint32_t linear = ((uint32_t)DGU16(entry + 8) << 4)
                          + DGU16(entry + 6)
                          + (((uint32_t)DGU16(entry + 0xc) << 16)
                             | DGU16(entry + 0xa));
        uint32_t p = normalise_far_ptr_far((uint16_t)(linear & 0xf),
                                           (uint16_t)(linear >> 4));

        DG16(0x589a) = (int16_t)(p >> 16);
        DG16(0x5898) = (int16_t)(p & 0xFFFF);
    }
    return 1;
}
/*
 * 0x1c389
 *
 * The next byte of whatever is being decompressed, or -1 at the end.
 *
 * There are two sources and the bit 0x20 at DGROUP 0x5888 chooses between them:
 * the resource file through `game_fgetc`, or a block already in memory, walked
 * by the huge pointer at DGROUP 0x5898 with `huge_post_add`.
 *
 * Either way the position at +0xa:+0xc of the record at DGROUP 0x588a is
 * stepped first, and the end test compares it against +0xe:+0x10 - so the count
 * is kept by the record and not by the source.
 *
 * The byte is zero-extended: `cbw` then `and ax,0xff`, which is the compiler
 * widening a `char` and then masking the sign back off.
 */
int16_t next_input_byte(void)
{
    uint16_t rec = DGU16(0x588a);

    if (DGU16(rec + 0xc) == DGU16(rec + 0x10)
        && DGU16(rec + 0xa) == DGU16(rec + 0xe))
        return -1;

    DG16(rec + 0xa) = (int16_t)(DGU16(rec + 0xa) + 1);
    if (DGU16(rec + 0xa) == 0)
        DG16(rec + 0xc) = (int16_t)(DGU16(rec + 0xc) + 1);

    if ((DG8(0x5888) & 0x20) != 0)
        return game_fgetc(DGU16(0x57bc));

    {
        uint32_t p = huge_post_add(0x5898, DGROUP_SEG, 1);

        return (int16_t)(*FAR_PTR((uint16_t)(p >> 16), (uint16_t)p) & 0xff);
    }
}

/*
 * 0x1c6e3
 *
 * Does this NUL-terminated string contain the letter `r`?
 *
 * A **near** function - it ends in `ret`, not `retf` - so its argument sits at
 * [bp+4] and the string is a DGROUP offset. The loop tests for the terminator
 * before each character and steps the pointer before testing it, so an empty
 * string answers no without reading anything.
 */
int16_t string_contains_r(uint16_t str)
{
    while (DG8(str) != 0) {
        uint16_t at = str;
        str++;
        if (DG8(at) == 'r')
            return 1;
    }
    return 0;
}
/*
 * 0x1c705
 *
 * Free a pointer unless it is null - the whole routine.
 *
 * A **near** call taking a near pointer, so its argument sits at [bp+4] rather
 * than the [bp+6] a far routine would use.
 *
 * The free itself is the C runtime's, which the port does not have; see
 * `io_malloc` in io.c for why it refuses rather than pretending.
 *
 * **Measured: the free path is reached on these screens.** So unlike the
 * allocator calls in the sound module, this one cannot be verified by
 * exercising only the other branch - it is not in tools/verify.py's list at
 * all, and will go in once the runtime's own allocator is transcribed.
 */
void free_if_set(uint16_t p)
{
    if (p != 0)
        io_free(p);
}
/*
 * 0x1c8a7
 *
 * Hand over the next run of bytes from the selected resource, up to whatever
 * the caller still wants. A **near** call taking nothing: everything is in the
 * globals `select_resource` set up.
 *
 * The entry's bytes at +0x1a and +0x1b are an end and a start, and their
 * difference is what is available. If that is more than the outstanding count
 * at 0x5890, only that much is taken and the start is advanced - by the **low
 * byte** of the count, because the start is a byte and the count is a word.
 * Otherwise the run is exhausted and both bytes are zeroed.
 *
 * The copy happens only with bit 0x40 set at 0x57ba. Without it the counters
 * still move, so a caller can walk a resource without reading it - which is how
 * a seek is done here.
 *
 * The destination far pointer at 0x5894 is advanced by the same amount through
 * the runtime's in-place huge-pointer add at 0x0be82; the port does the linear
 * arithmetic and renormalises, which is what that routine amounts to.
 */
void resource_advance(void)
{
    uint16_t entry = DGU16(0x588a);
    uint16_t di = DG8(entry + 0x1b);
    uint16_t si = (uint16_t)(DG8(entry + 0x1a) - di);

    if (si > DGU16(0x5890)) {
        si = DGU16(0x5890);
        DG8(entry + 0x1b) = (uint8_t)(DG8(entry + 0x1b) + (uint8_t)si);
    } else {
        DG8(entry + 0x1a) = 0;
        DG8(entry + 0x1b) = 0;
    }

    if (si == 0)
        return;

    if ((DG8(0x57ba) & 0x40) != 0)
        far_memcpy(DGU16(0x5894), DGU16(0x5896),
                   (uint16_t)(DGU16(0x5892) + di),
                   (uint16_t)(dgroup_base >> 4), si);

    DG16(0x5890) = (int16_t)(DGU16(0x5890) - si);

    {
        uint32_t linear = ((uint32_t)DGU16(0x5896) << 4) + DGU16(0x5894) + si;

        DG16(0x5896) = (int16_t)(linear >> 4);
        DG16(0x5894) = (int16_t)(linear & 0xf);
    }
}
/*
 * 0x1dfd6
 *
 * One bit of the type-3 stream, as 0 or 1.
 *
 * The buffer is a word at DGROUP 0x3600 filled from the top, with the number of
 * bits in it at 0x3602. Bits come off the **left**: the answer is the sign of
 * the word, and the word is then shifted up by one.
 *
 * A refill happens whenever there are eight or fewer bits, not when the buffer
 * is empty, so there is always a whole byte's headroom. `next_input_byte`
 * answers -1 at the end of the input and only its low byte is taken, so the
 * stream runs on into 0xff bytes rather than stopping - which is what the
 * decoder above expects, since it stops on a symbol and not on the input.
 */
int16_t huff_get_bit(void)
{
    int16_t si;

    if (DG8(0x3602) <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - DG8(0x3602)));
        DG16(0x3600) = (int16_t)(DGU16(0x3600) | ax);
        DG8(0x3602) = (uint8_t)(DG8(0x3602) + 8);
    }

    si = DG16(0x3600);
    DG16(0x3600) = (int16_t)(DGU16(0x3600) << 1);
    DG8(0x3602) = (uint8_t)(DG8(0x3602) - 1);

    return (int16_t)(si < 0 ? 1 : 0);
}

/*
 * 0x1e00b
 *
 * Eight bits of the same stream, as a byte. The same buffer at DGROUP 0x3600
 * and the same refill rule, but the refill is a **loop** here: taking eight
 * bits at once can need two bytes in, where `huff_get_bit` never needs more
 * than one.
 */
int16_t huff_get_byte(void)
{
    uint16_t si;

    while (DG8(0x3602) <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - DG8(0x3602)));
        DG16(0x3600) = (int16_t)(DGU16(0x3600) | ax);
        DG8(0x3602) = (uint8_t)(DG8(0x3602) + 8);
    }

    si = DGU16(0x3600);
    DG16(0x3600) = (int16_t)(si << 8);
    DG8(0x3602) = (uint8_t)(DG8(0x3602) - 8);

    return (int16_t)(si >> 8);
}

/*
 * 0x1e0b3
 *
 * Build the adaptive Huffman tree that decompression type 3 decodes with.
 *
 * Three arrays live inside the record at DGROUP 0x588a, at +0x103b, +0x1523
 * and +0x1c7d, and their far pointers are cached at DGROUP 0x590a, 0x590e and
 * 0x5900. They abut exactly: 0x274 words of frequency, then the parent array,
 * then the son array.
 *
 * The shape is LZHUF's, and the constants say so: 0x13a symbols, a tree of
 * 0x273 nodes with the root at 0x272 - 314, 627 and 626. Every leaf starts with
 * a frequency of 1 and is hung under a parent built by pairing leaves upward,
 * and the two sentinels at the end - a frequency of 0xffff past the root and a
 * parent of 0 at it - are what stop the update walk later.
 *
 * The identification is from the structure and the constants, not from any
 * source: this is transcribed from the bytes like everything else here.
 */
void huffman_start(void)
{
    uint16_t rec = DGU16(0x588a);
    uint16_t seg = DGU16(rec + 4);
    uint16_t freq, prnt, son;
    int16_t i, j;

    DG16(0x590c) = (int16_t)seg;
    DG16(0x590a) = (int16_t)(DGU16(rec + 2) + 0x103b);
    DG16(0x5910) = (int16_t)seg;
    DG16(0x590e) = (int16_t)(DGU16(rec + 2) + 0x1523);
    DG16(0x5902) = (int16_t)seg;
    DG16(0x5900) = (int16_t)(DGU16(rec + 2) + 0x1c7d);

    freq = DGU16(0x590a);
    prnt = DGU16(0x590e);
    son  = DGU16(0x5900);

    for (i = 0; i < 0x13a; i++) {
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(freq + 2 * i)) = 1;
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(son + 2 * i)) =
            (uint16_t)(i + 0x273);
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(prnt + 2 * (i + 0x273))) =
            (uint16_t)i;
    }

    i = 0;
    for (j = 0x13a; j <= 0x272; j++) {
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(freq + 2 * j)) =
            (uint16_t)(*(uint16_t *)FAR_PTR(seg, (uint16_t)(freq + 2 * i))
                       + *(uint16_t *)FAR_PTR(seg,
                                              (uint16_t)(freq + 2 * (i + 1))));
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(son + 2 * j)) = (uint16_t)i;
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(prnt + 2 * (i + 1))) =
            (uint16_t)j;
        *(uint16_t *)FAR_PTR(seg, (uint16_t)(prnt + 2 * i)) = (uint16_t)j;
        i += 2;
    }

    *(uint16_t *)FAR_PTR(seg, (uint16_t)(freq + 0x4e6)) = 0xffff;
    *(uint16_t *)FAR_PTR(seg, (uint16_t)(prnt + 0x4e4)) = 0;
}

/*
 * 0x1e1af
 *
 * Halve every frequency and rebuild the tree, when the root's count would
 * overflow. LZHUF's `reconst`.
 *
 * **It is reached by a jump, not a call.** `huffman_update` jumps here and this
 * ends with a jump back into the middle of it, having built and torn down its
 * own frame in between. The port makes it an ordinary function, which is what
 * it is everywhere except in the two instructions that connect them.
 *
 * Three passes. The leaves are collected to the front with their frequencies
 * rounded up and halved; the internal nodes are rebuilt by pairing and each
 * insertion point found by walking back until the frequency fits; then every
 * parent link is written from the son array.
 *
 * **The shift in the second pass moves twice as many entries as it should.**
 * The count is `(j - k) * 2`, which is right as a *byte* count for a `memmove`
 * and wrong as the element count this loop uses it as, so it walks up to
 * `2j - k` instead of `j`. Nothing is lost by it: every entry above `j` is
 * recomputed by a later turn of the outer loop before anything reads it. It is
 * transcribed as it behaves.
 */
void huffman_reconst(void)
{
    uint16_t seg = DGU16(0x590c);
    uint16_t freq = DGU16(0x590a);
    uint16_t prnt = DGU16(0x590e);
    uint16_t son = DGU16(0x5900);
    int16_t i, j, k, n;

#define FREQ(x) (*(uint16_t *)FAR_PTR(seg, (uint16_t)(freq + 2 * (x))))
#define PRNT(x) (*(uint16_t *)FAR_PTR(seg, (uint16_t)(prnt + 2 * (x))))
#define SON(x)  (*(uint16_t *)FAR_PTR(seg, (uint16_t)(son  + 2 * (x))))

    j = 0;
    for (i = 0; i < 0x273; i++) {
        if (SON(i) >= 0x273) {
            FREQ(j) = (uint16_t)((FREQ(i) + 1) >> 1);
            SON(j) = SON(i);
            j++;
        }
    }

    i = 0;
    for (j = 0x13a; j < 0x273; j++) {
        uint16_t f = (uint16_t)(FREQ(i) + FREQ(i + 1));

        FREQ(j) = f;

        for (k = (int16_t)(j - 1); FREQ(k) > f; k--)
            ;
        k++;

        for (n = (int16_t)((j - k) * 2 - 1); n >= 0; n--) {
            FREQ(k + n + 1) = FREQ(k + n);
            SON(k + n + 1) = SON(k + n);
        }

        FREQ(k) = f;
        SON(k) = (uint16_t)i;
        i += 2;
    }

    for (i = 0; i < 0x273; i++) {
        uint16_t c = SON(i);

        if (c >= 0x273) {
            PRNT(c) = (uint16_t)i;
        } else {
            PRNT(c + 1) = (uint16_t)i;
            PRNT(c) = (uint16_t)i;
        }
    }
}

/*
 * 0x1e338
 *
 * Count one symbol and keep the tree ordered. LZHUF's `update`.
 *
 * The walk starts at the symbol's leaf parent and goes up to the root, adding
 * one at each step. When a node's new count passes its right-hand neighbour's
 * the two are swapped - frequencies, sons, and both parent links each, since an
 * internal node's two children share a parent entry.
 *
 * A root count of 0x8000 sends it to `huffman_reconst` first, which is why
 * frequencies never overflow.
 */
void huffman_update(uint16_t c)
{
    uint16_t seg = DGU16(0x590c);
    uint16_t freq = DGU16(0x590a);
    uint16_t prnt = DGU16(0x590e);
    uint16_t son = DGU16(0x5900);

    if (FREQ(0x272) == 0x8000)
        huffman_reconst();

    c = PRNT(c + 0x273);

    do {
        uint16_t k = (uint16_t)(FREQ(c) + 1);
        uint16_t l = (uint16_t)(c + 1);

        FREQ(c) = k;

        if (FREQ(l) < k) {
            uint16_t i, j;

            while (FREQ(l) < k)
                l++;
            l--;

            FREQ(c) = FREQ(l);
            FREQ(l) = k;

            i = SON(c);
            PRNT(i) = l;
            if (i < 0x273)
                PRNT(i + 1) = l;

            j = SON(l);
            SON(l) = i;
            PRNT(j) = c;
            if (j < 0x273)
                PRNT(j + 1) = c;
            SON(c) = j;

            c = l;
        }

        c = PRNT(c);
    } while (c != 0);

#undef FREQ
#undef PRNT
#undef SON
}

/*
 * 0x1e561
 *
 * Decode a match position: twelve bits, of which the top six come out of a
 * table and the bottom six are read raw.
 *
 * A byte is taken first and used to index two 256-entry tables in DGROUP - the
 * code at 0x3686 and the length at 0x3786 - which between them say how many
 * further bits the position needs. Those bits are shifted into the byte one at
 * a time, and only the low six of the result survive; the table's code supplies
 * the rest, shifted up by six.
 *
 * **It is reached by a jump, not a call**, and ends with a jump back into
 * 0x1e7f2 rather than a `ret` - the same arrangement as `huffman_reconst`. The
 * port makes it an ordinary function. That is also why the verifier cannot
 * check it on its own: there is no return for the harness to watch for. Every
 * one of `decompress_lzss`'s 226 verified calls runs it.
 */
int16_t decode_position(void)
{
    uint16_t si = (uint16_t)huff_get_byte();
    uint16_t high = (uint16_t)(DG8(0x3686 + si) << 6);
    int16_t n = (int16_t)(DG8(0x3786 + si) - 2);

    while (n-- != 0)
        si = (uint16_t)(2 * si + huff_get_bit());

    return (int16_t)(high | (si & 0x3f));
}

/*
 * 0x1e7f2
 *
 * Decompression type 3: LZSS over a 4096-byte ring, with the literals and match
 * lengths adaptively Huffman coded and the match positions coded by
 * `decode_position`. The third and busiest of the handlers the table at DGROUP
 * 0x3580 dispatches to.
 *
 * Like `decompress_lzw` it can stop in the middle and be called again, and for
 * the same reason: `emit_byte` answers 0 once the caller's request is full. The
 * flag at DGROUP 0x58e0 says a match was interrupted, and the position, length
 * and progress at 0x58e2, 0x58e4 and 0x58e6 are what it comes back to.
 *
 * The first call also initialises: the tree, the ring filled with 0xfc4 spaces
 * and the write position set past them, and the total to produce read from the
 * record's +0x12:+0x14. 0x5918 is what makes that happen once.
 *
 * A symbol below 0x100 is a literal. Anything else is a match, whose length is
 * the symbol less 0xfd and whose source is the ring position that far back,
 * masked to twelve bits. Every byte produced goes to the output *and* back into
 * the ring, which is why a match may read bytes it has just written.
 *
 * Two blocks of this routine are placed out of line by the compiler and are
 * folded back in here: the symbol decode at 0x1e52d, which walks the tree from
 * the root a bit at a time, and 0x1e561 above.
 *
 * The answer is always 0.
 */
int16_t decompress_lzss(void)
{
    uint16_t di = 0;
    int16_t si;

    if (DG16(0x5918) == 0) {
        uint16_t rec;
        int16_t i;

        DG16(0x58e0) = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            *FAR_PTR(DGU16(0x5914),
                     (uint16_t)(DGU16(0x5912) + i)) = 0x20;

        DG16(0x58e8) = 0xfc4;
        DG16(0x58ec) = 0;
        DG16(0x58ea) = 0;

        rec = DGU16(0x588a);
        DG16(0x58f0) = DG16(rec + 0x14);
        DG16(0x58ee) = DG16(rec + 0x12);
        DG16(0x5918) = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (DG16(0x58ec) >= DG16(0x58f0)
            && (DG16(0x58ec) != DG16(0x58f0)
                || DGU16(0x58ea) >= DGU16(0x58ee)))
            return 0;

        if (DG16(0x58e0) == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            uint16_t son = DGU16(0x5900);
            uint16_t seg = DGU16(0x5902);

            di = *(uint16_t *)FAR_PTR(seg, (uint16_t)(son + 0x4e4));
            while (di < 0x273)
                di = *(uint16_t *)FAR_PTR(
                    seg, (uint16_t)(son + 2 * (di + huff_get_bit())));

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                *FAR_PTR(DGU16(0x5914),
                         (uint16_t)(DGU16(0x5912) + DGU16(0x58e8))) =
                    (uint8_t)di;
                DG16(0x58e8) = (int16_t)((DGU16(0x58e8) + 1) & 0xfff);
                DG16(0x58ea) = (int16_t)(DGU16(0x58ea) + 1);
                if (DGU16(0x58ea) == 0)
                    DG16(0x58ec) = (int16_t)(DGU16(0x58ec) + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                DG16(0x58e2) = (int16_t)((DGU16(0x58e8) - pos - 1) & 0xfff);
                DG16(0x58e4) = (int16_t)(di + 0xff03);
                DG16(0x58e6) = 0;
            }
        }

        DG16(0x58e0) = 0;

        while (DG16(0x58e6) < DG16(0x58e4)) {
            uint16_t b = *FAR_PTR(
                DGU16(0x5914),
                (uint16_t)(DGU16(0x5912)
                           + ((DGU16(0x58e2) + DGU16(0x58e6)) & 0xfff)));

            si = emit_byte(b);

            *FAR_PTR(DGU16(0x5914),
                     (uint16_t)(DGU16(0x5912) + DGU16(0x58e8))) = (uint8_t)b;
            DG16(0x58e8) = (int16_t)((DGU16(0x58e8) + 1) & 0xfff);
            DG16(0x58ea) = (int16_t)(DGU16(0x58ea) + 1);
            if (DGU16(0x58ea) == 0)
                DG16(0x58ec) = (int16_t)(DGU16(0x58ec) + 1);

            DG16(0x58e6) = (int16_t)(DGU16(0x58e6) + 1);

            if (si == 0) {
                DG16(0x58e0) = 1;
                return 0;
            }
        }
    }
}

/*
 * 0x1eb6a
 *
 * Set the current palette, or answer the one already set.
 *
 * It first makes sure a buffer exists: the byte at VMDS+0x1d - the driver's
 * own mode number, sign extended - indexes a table of sizes at DGROUP 0x4466,
 * and if the far pointer at 0x3a2e is still null a block of twice that many
 * bytes is allocated for it.
 *
 * Then, with a null argument it answers the pointer it last stored; with a
 * real one it stores it, hands it to the driver at VGA:0x0f15 through the
 * vector at DGROUP 0x4396, and answers it back.
 *
 * The pointer is passed and answered offset-first, in AX, with the segment in
 * DX - the usual far-pointer convention here.
 */
uint32_t set_palette_pointer(uint16_t off, uint16_t seg)
{
    int16_t idx = (int8_t)DG8(VMDS + 0x1D);

    DG16(0x4464) = DG16((uint16_t)(0x4466 + idx * 2));

    if ((uint16_t)(DGU16(0x3A2E) | DGU16(0x3A30)) == 0 && DG16(0x4464) != 0) {
        int16_t bytes = (int16_t)(DG16(0x4464) * 2);
        uint32_t p = dos_alloc_bytes((uint16_t)bytes,
                                     (uint16_t)(bytes < 0 ? 0xFFFF : 0), 0, 0);
        DGU16(0x3A30) = (uint16_t)(p >> 16);
        DGU16(0x3A2E) = (uint16_t)p;
    }

    if ((uint16_t)(off | seg) == 0)
        return ((uint32_t)DGU16(0x44C4) << 16) | DGU16(0x44C2);

    DGU16(0x44C4) = seg;
    DGU16(0x44C2) = off;
    vm_load_palette(off, seg);
    return ((uint32_t)seg << 16) | off;
}/*
 * 0x20079
 *
 * Fill a rectangle, clipped, and optionally outline it.
 *
 * The fill is done by turning the rectangle into a **span list** - the first
 * row, the row count, then one `x1, x2` pair per row, all identical - and
 * handing it to the driver at VGA:0x0be6 through the vector at DGROUP 0x43b2.
 * That is a general span filler being used for the simplest possible case,
 * which is why a solid rectangle costs one entry per scan line.
 *
 * Clipping shrinks the rectangle in place against the box at DGROUP
 * 0x3894..0x389a, and the original x and y are pushed before that and popped
 * back afterwards, because the outline below wants the unclipped ones.
 *
 * The outline at 0x2013f is **not transcribed**. It draws four lines through
 * 0x21e34 with a stack-reuse trick - each call pushes only the arguments that
 * differ from the last and relies on the rest still being there - which has no
 * honest expression in C without modelling the stack. It is never reached on
 * the intro screens: over 2,108 calls the two border bytes at DGROUP
 * 0x389d/0x389e - the driver's own colour bytes, seen through DGROUP - were
 * always equal, which is the condition that skips it.
 */
void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t right = (int16_t)(x + w - 1);
    int16_t bottom = (int16_t)(y + h - 1);

    if (fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (clip_enabled != 0) {
            int16_t d = (int16_t)(cx - clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = FAR_PTR(span_buffer_seg, 0);
            int16_t n = ch;
            int16_t x2 = (int16_t)(cx + cw - 1);

            *p++ = (uint8_t)(cy & 0xFF);
            *p++ = (uint8_t)((uint16_t)cy >> 8);
            *p++ = (uint8_t)(ch & 0xFF);
            *p++ = (uint8_t)((uint16_t)ch >> 8);
            do {
                *p++ = (uint8_t)(cx & 0xFF);
                *p++ = (uint8_t)((uint16_t)cx >> 8);
                *p++ = (uint8_t)(x2 & 0xFF);
                *p++ = (uint8_t)((uint16_t)x2 >> 8);
            } while (--n);

            vm_fill_spans(span_buffer_seg, 0);
        }
    }

    if (fill_enabled != 0 && vga_fill_colour == vga_second_colour)
        return;
    not_transcribed("0x2013f, the rectangle outline");
}
/*
 * 0x20654
 *
 * Take a slot in the timer's callback table and fill it in. Answers the slot
 * number plus one - so 1..8, with 0 meaning it could not.
 *
 * Two things stop it: a **zero** byte at DGROUP 0x44ee, which is the flag
 * saying the timer handler is not installed - 0x206c1 installs only while it is
 * zero and sets it, and this registers only once it is set - and a full mask at
 * 0x44f7, tested as `mask + 1 == 0` rather than against 0xffff, which is the
 * same thing in one instruction.
 *
 * The free slot is found by shifting the mask right until a zero bit falls out,
 * counting `BX` up in fours and `CX` along as the bit. The four parallel tables
 * are therefore indexed by `slot * 4`: the far pointer at 0x44f9 and 0x44fb,
 * and the reload count at 0x4539 with its running copy at 0x453b - both set to
 * the same value here, so the first tick is a whole period away.
 *
 * The mask is set with interrupts off, because the handler reads it.
 *
 * Hand-written assembly: no locals, and `AX` is the answer throughout.
 */
uint16_t timer_add_callback(uint16_t off, uint16_t seg, uint16_t period)
{
    uint16_t mask, bx, cx;

    if (DG8(0x44ee) == 0)
        return 0;

    mask = DGU16(0x44f7);
    if ((uint16_t)(mask + 1) == 0)
        return 0;

    bx = 0;
    cx = 1;
    while ((mask & 1) != 0) {
        mask >>= 1;
        cx = (uint16_t)(cx << 1);
        bx = (uint16_t)(bx + 4);
    }

    DG16(bx + 0x453b) = (int16_t)period;
    DG16(bx + 0x4539) = (int16_t)period;
    DG16(bx + 0x44f9) = (int16_t)off;
    DG16(bx + 0x44fb) = (int16_t)seg;

    DG16(0x44f7) = (int16_t)(DGU16(0x44f7) | cx);

    return (uint16_t)((bx >> 2) + 1);
}

/*
 * 0x2069e
 *
 * Give a timer slot back: clear its bit in the mask at DGROUP 0x44f7. Answers 1
 * if it did, 0 if the handle was out of range.
 *
 * The handle is the slot plus one, and the range test is `(handle - 1) & 0xf0`
 * - so it admits 1..16 while only eight slots exist. Clearing a bit above the
 * eighth is harmless, since nothing reads it.
 *
 * The mask of everything-but-one bit is built rather than looked up: `stc`,
 * then 0xfffe rotated **left through carry** by the slot number, which walks
 * the single zero up and feeds ones in behind it.
 *
 * Hand-written assembly, no locals.
 */
uint16_t timer_drop_callback(uint16_t handle)
{
    uint16_t cl = (uint16_t)((handle - 1) & 0xff);
    uint16_t v;
    int16_t i;
    uint16_t carry;

    if ((cl & 0xf0) != 0)
        return 0;

    v = 0xfffe;
    carry = 1;
    for (i = 0; i < (int16_t)cl; i++) {
        uint16_t out = (uint16_t)(v >> 15);

        v = (uint16_t)((v << 1) | carry);
        carry = out;
    }

    DG16(0x44f7) = (int16_t)(DGU16(0x44f7) & v);

    return 1;
}

/*
 * 0x2147d
 *
 * Return bit 0 of the byte at DGROUP 0x468c + index.
 *
 * It runs with interrupts disabled, so the array is something an interrupt
 * handler also writes - a keyboard or timer flag, most likely, though that is
 * inference and not established.
 *
 * It does **not** push BP: it saves it in DX and points BP at the stack, so
 * its argument is at [bp+4] rather than the usual [bp+6]. Transcribed as an
 * ordinary parameter, since the port has no BP to preserve.
 */
int16_t bit0_of_468c(uint16_t index)
{
    return (int16_t)(byte_array_468c(index) & 1);
}
/*
 * 0x21abd
 *
 * Allocate memory from DOS, given a **32-bit byte count**, and answer a far
 * pointer to it in DX:AX - always at offset 0, since DOS hands out whole
 * paragraphs.
 *
 * The size is turned into paragraphs by shifting the pair right four times
 * with `shr`/`rcr`, and rounded **up** if any of the low four bits were set -
 * the remainder is tested from a copy taken before the shifting.
 *
 * A size of 0xffffffff is not a request but a question: it calls DOS with
 * 0xffff paragraphs, which always fails, and converts the largest-free figure
 * DOS reports back into bytes. So one routine both allocates and asks how much
 * there is, told apart by its argument.
 *
 * Bit 0 of the flags asks for the block to be zeroed, which it does through
 * `far_memset` at 0x22300. The flags are the **fourth** argument, at [bp+0xc];
 * the third is pushed by every caller and never read. Reading the third as the
 * flags was an error here that verified anyway, because the callers seen so
 * far push zero into both.
 *
 * The DOS call itself is IO - see io.h - and is primed by the verifier with
 * what DOS actually answered, because the port has no arena of its own.
 */
uint32_t dos_alloc_bytes(uint16_t size_lo, uint16_t size_hi,
                         uint16_t unused, uint16_t flags)
{
    (void)unused;
    uint16_t paras_lo, paras_hi, remainder, seg, largest;
    int32_t failed;

    if (size_hi == size_lo && size_hi == 0xFFFF) {
        /* The "how much is free" question. */
        io_dos_alloc(0xFFFF, &largest, &failed);
        {
            uint32_t bytes = (uint32_t)largest << 4;
            return bytes;
        }
    }

    remainder = (uint16_t)(size_lo & 0x0F);
    paras_hi = size_hi;
    paras_lo = size_lo;
    {
        int32_t i;
        for (i = 0; i < 4; i++) {
            paras_lo = (uint16_t)((paras_lo >> 1) | ((paras_hi & 1) << 15));
            paras_hi = (uint16_t)(paras_hi >> 1);
        }
    }
    if (remainder != 0)
        paras_lo = (uint16_t)(paras_lo + 1);

    seg = io_dos_alloc(paras_lo, &largest, &failed);
    if (failed)
        return 0;

    if (flags & 1)
        far_memset(0, seg, 0, size_lo, size_hi);

    return (uint32_t)seg << 16;
}
/*
 * 0x21b34
 *
 * Hand a block back to DOS - INT 21h with AH=0x49 and the block's segment in
 * ES.
 *
 * The argument is a **far pointer**, and only its segment half is used: the
 * routine reads [bp+8], the second word, and never looks at the offset at
 * [bp+6]. DOS hands out whole paragraphs at offset zero, so the offset carries
 * no information to begin with.
 *
 * Nothing checks the result. DOS reports failure in CF with an error code in
 * AX, and the routine returns whatever DOS left there without looking, so a
 * double free or a corrupted arena passes silently.
 *
 * The DOS call is IO - see io.h. The port has no arena to give the block back
 * to, so this changes no guest memory.
 */
void dos_free_far(uint16_t off, uint16_t seg)
{
    (void)off;
    io_dos_free(seg);
}
/*
 * 0x21e34
 *
 * Clip a line to the clip box and hand what is left to the driver's line
 * drawer through the vector at DGROUP 0x434e.
 *
 * Four stages - top, left, bottom, right - each the same shape: if both ends
 * are outside the edge the line is dropped entirely; if both are inside the
 * stage is skipped; otherwise the two ends are **swapped** so the outside one
 * is first, and it is moved onto the edge by interpolation.
 *
 * The interpolation is a 32-bit intermediate: `imul` makes a 32-bit product in
 * DX:AX and `idiv` divides it, so a long multiply is essential here and doing
 * it in 16 bits would overflow on a long line.
 *
 * **The first two stages compare signed and the last two unsigned** - `jl`/
 * `jge` against top and left, `ja`/`jbe` against bottom and right. That is not
 * a slip to tidy: once a line has been clipped to the top and left edges its
 * coordinates cannot be negative, so unsigned compares are safe and shorter.
 * Transcribed with the same signedness.
 *
 * BP is used as a scratch register for the divisor, which destroys the frame
 * pointer - safe only because every argument has already been loaded into a
 * register by then.
 *
 * Finally the ends are ordered by x, swapping both coordinates together, so
 * the drawer always receives them left to right.
 */
void clip_and_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    int16_t edge, t;

    if (clip_enabled != 0) {
        /* top */
        edge = clip_top;
        if (y1 < edge) {
            if (y2 < edge)
                return;
        } else if (y2 >= edge) {
            goto left;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

left:
        edge = clip_left;
        if (x1 < edge) {
            if (x2 < edge)
                return;
        } else if (x2 >= edge) {
            goto bottom;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;

bottom:
        edge = clip_bottom;
        if ((uint16_t)y1 > (uint16_t)edge) {
            if ((uint16_t)y2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)y2 <= (uint16_t)edge) {
            goto right;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

right:
        edge = clip_right;
        if ((uint16_t)x1 > (uint16_t)edge) {
            if ((uint16_t)x2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)x2 <= (uint16_t)edge) {
            goto draw;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;
    }

draw:
    if ((uint16_t)x1 > (uint16_t)x2) {
        t = x1; x1 = x2; x2 = t;
        t = y1; y1 = y2; y2 = t;
    }
    vm_draw_line(x1, y1, x2, y2);
}
/*
 * 0x220e9
 *
 * If the flag byte at DGROUP 0x48ea is set, store a quarter of each of the two
 * words at DGROUP 0x4740 and 0x4742 through the two near pointers passed in.
 * If it is clear, both are left alone - the routine writes nothing at all,
 * which a caller that did not initialise them would notice.
 *
 * The `neg`/`jae` pair again: carry is set exactly when the byte was non-zero.
 */
void read_pair_4740(uint16_t out_a, uint16_t out_b)
{
    if (DG8(0x48EA) == 0)
        return;
    DG16(out_a) = (int16_t)(DGU16(0x4740) >> 2);
    DG16(out_b) = (int16_t)(DGU16(0x4742) >> 2);
}
/*
 * 0x2213e
 *
 * Answer bit 0 of one of two flag bytes, or 0 if the first of them is clear.
 *
 * The original tests with `neg` and `jae`: `neg` leaves the carry flag set
 * exactly when its operand was non-zero, which is how this reads a byte and
 * branches on it without a compare. When DGROUP 0x48ea is zero the negation
 * leaves zero and the final AND answers 0; otherwise the byte at 0x48eb is
 * taken, shifted right once if the argument is non-zero, and its bit 0
 * returned.
 */
int16_t flag_bit_48ea(uint16_t which)
{
    uint16_t v = DG8(0x48EA);

    if (v == 0)
        return 0;

    v = DG8(0x48EB);
    if (which != 0)
        v >>= 1;
    return (int16_t)(v & 1);
}
/*
 * 0x22161
 *
 * **Normalise a far pointer**: carry the paragraphs out of the offset into the
 * segment, leaving an offset of at most 15. AX holds the offset and DX the
 * segment; `AX >> 4` paragraphs move into DX and the offset is masked to four
 * bits.
 *
 * *This was first transcribed under the name `fixed_normalise`, as though AX
 * held a fixed-point fraction.* The behaviour is the same either way - it is
 * the same six instructions - but the name was wrong, and 0x222c6 is what
 * settled it: that routine calls this on the offset/segment halves of two far
 * pointers and then copies between them with `movsb`, which only makes sense
 * for pointer normalisation. A wrong name survives longer than a wrong line,
 * so it is corrected here and the correction recorded.
 *
 * A **near** routine taking and answering registers, so the port passes them
 * by reference. Both reads use the original AX, which is why the addition is
 * done before the mask.
 */
void normalise_far_ptr(uint16_t *off, uint16_t *seg)
{
    uint16_t ax = *off;

    *seg = (uint16_t)(*seg + (ax >> 4));
    *off = (uint16_t)(ax & 0x0F);
}
/*
 * 0x222c6
 *
 * Copy `count` bytes between two far pointers, normalising both first so that
 * each offset is under 16 and the segment carries the paragraphs.
 *
 * **The alignment step is dead code, in the original.** It reads
 *
 *     test di, 1
 *     jae  skip
 *     movsb
 *     dec  cx
 *
 * and `test` always clears the carry flag, so `jae` is always taken: the byte
 * that would have aligned the destination is never copied. It was presumably
 * meant to be `jz`. Transcribed as it behaves, not as it was meant, with the
 * dead branch recorded here rather than silently reinstated.
 *
 * The tail is `shr cx,1 / rep movsw / rcl cx,1 / rep movsb`: the shift puts the
 * odd bit into carry, the words are copied, and the rotate brings that bit back
 * into a count of 0 or 1 for the trailing byte. No compare anywhere.
 */
void far_memcpy(uint16_t dst_off, uint16_t dst_seg,
                uint16_t src_off, uint16_t src_seg, uint16_t count)
{
    uint16_t words;

    if (count == 0)
        return;

    normalise_far_ptr(&src_off, &src_seg);
    normalise_far_ptr(&dst_off, &dst_seg);

    words = (uint16_t)(count >> 1);
    while (words--) {
        FARU16(dst_seg, dst_off) = FARU16(src_seg, src_off);
        src_off = (uint16_t)(src_off + 2);
        dst_off = (uint16_t)(dst_off + 2);
    }
    if (count & 1)
        FAR8(dst_seg, dst_off) = FAR8(src_seg, src_off);
}
/*
 * 0x22300
 *
 * Fill memory through a far pointer, with a **32-bit** count, in chunks of at
 * most 0x7d00 bytes so that a chunk can never carry the offset past 64K. The
 * pointer is renormalised at the top of every chunk, and the count is reduced
 * by `sub`/`sbb` across both halves.
 *
 * **The alignment test is wrong, in the original.** It reads
 *
 *     or  di, di
 *     jp  skip
 *     stosb
 *
 * and `jp` is jump-if-**parity**, not jump-if-odd: `or di,di` sets PF from the
 * parity of DI's low byte, which has nothing to do with whether the address is
 * even. So the byte that would align the destination is stored or not
 * according to how many bits are set in the low half of the address. It is
 * transcribed as it behaves - the parity of the low byte - rather than as the
 * alignment test it was meant to be. `far_memcpy` at 0x222c6 has a bug in the
 * same place, differently: there the branch is always taken.
 *
 * A chunk shorter than ten bytes skips the word-fill entirely and is done with
 * `rep stosb`.
 */
void far_memset(uint16_t off, uint16_t seg, uint16_t value,
                uint16_t count_lo, uint16_t count_hi)
{
    uint16_t pair = (uint16_t)((value & 0xFF) | ((value & 0xFF) << 8));

    for (;;) {
        uint16_t chunk = 0x7D00;
        uint16_t taken, cx;

        if (count_hi == 0) {
            if (count_lo == 0)
                return;
            if ((int16_t)count_lo <= (int16_t)0x7D00)
                chunk = count_lo;
        }
        taken = chunk;

        normalise_far_ptr(&off, &seg);
        cx = chunk;

        if ((int16_t)cx >= 0x0A) {
            if (!low_byte_parity_even(off)) {
                FAR8(seg, off) = (uint8_t)value;
                off = (uint16_t)(off + 1);
                cx--;
            }
            {
                uint16_t words = (uint16_t)(cx >> 1);
                uint16_t odd = (uint16_t)(cx & 1);

                while (words--) {
                    FARU16(seg, off) = pair;
                    off = (uint16_t)(off + 2);
                }
                cx = odd;
            }
        }
        while (cx--) {
            FAR8(seg, off) = (uint8_t)value;
            off = (uint16_t)(off + 1);
        }

        {
            uint32_t total = ((uint32_t)count_hi << 16) | count_lo;
            total -= taken;
            count_lo = (uint16_t)total;
            count_hi = (uint16_t)(total >> 16);
        }
    }
}
/*
 * 0x22386
 *
 * The far-callable face of `normalise_far_ptr` at 0x22161: load the pointer
 * into AX and DX, call the near routine, and let its registers be the result.
 * So this answers a normalised far pointer in DX:AX, like any other far
 * routine returning a long.
 */
uint32_t normalise_far_ptr_far(uint16_t off, uint16_t seg)
{
    normalise_far_ptr(&off, &seg);
    return ((uint32_t)seg << 16) | off;
}
/*
 * 0x2241b
 *
 * Read a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The same guard as `plot_pixel_clipped` at 0x2244d, on the same four inclusive
 * bounds and the same on/off byte at 0x3893, differing only in taking two
 * arguments instead of three and jumping through DGROUP 0x439a. The two sit
 * next to each other in the image and are plainly one pair.
 *
 * -1 for "outside" is not distinguishable from a legitimately read colour only
 * because colours here are 0..15; any answer above that is the clip.
 */
int16_t read_pixel_clipped(int16_t x, int16_t y)
{
    if (DG8(0x3893) != 0) {
        if (x < DG16(0x3894))
            return -1;
        if (x > DG16(0x3896))
            return -1;
        if (y < DG16(0x3898))
            return -1;
        if (y > DG16(0x389a))
            return -1;
    }

    return (int16_t)vm_read_pixel(x, y);
}
/*
 * 0x2244d
 *
 * Plot a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The window is four words in the driver's data block - 0x3894 and 0x3896 for
 * x, 0x3898 and 0x389a for y - and both bounds are **inclusive**, tested with
 * `jl` and `jg`. The byte at 0x3893 switches the whole test off, and with it
 * zero any coordinate is passed straight through.
 *
 * The call into the driver is an `ljmp` through DGROUP 0x439e, not a call, so
 * the driver runs on this frame, sees the same three arguments, and returns
 * directly to whoever called here. The third argument is the colour and is
 * never looked at on the way past.
 *
 * That also means the answer on the drawn path is not chosen: it is whatever
 * the driver left in AX, which is 0xff08. Only the clipped path returns a
 * deliberate value.
 */
int16_t plot_pixel_clipped(int16_t x, int16_t y, int16_t colour)
{
    if (DG8(0x3893) != 0) {
        if (x < DG16(0x3894))
            return -1;
        if (x > DG16(0x3896))
            return -1;
        if (y < DG16(0x3898))
            return -1;
        if (y > DG16(0x389a))
            return -1;
    }

    return (int16_t)vm_plot_pixel(x, y, (uint8_t)colour);
}
