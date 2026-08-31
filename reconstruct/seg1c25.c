
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
 * 0x1c970
 *
 * Reset the LZW state for a new stream: the whole 0x3aa1-byte block cleared,
 * the code width back to nine with its limit at 0x1ff, the first 0x100 codes
 * made into single-byte strings - prefix zero, suffix the code itself - and the
 * next free code set to 0x101, one past the clear code.
 *
 * Every one of those writes goes through the huge-pointer add, because the
 * block is far and the tables run past a segment: the prefixes at twice the
 * code and the suffixes at 0x2720 plus it.
 *
 * The scratch pointer at DGROUP 0x58a8 is set to 0x3720 into the block, which
 * is where `decompress_lzw` builds each decoded string.
 *
 * The flag at 0x58ae says the next code read is the first, and 0x58a2 that no
 * byte is left over.
 */
void lzw_reset(void)
{
    int16_t i;
    uint32_t p;

    far_memset(DGU16(0x588c), DGU16(0x588e), 0, 0x3aa1, 0);

    DG16(0x589e) = 9;
    DG16(0x58b6) = (int16_t)((1 << 9) - 1);

    for (i = 0xff; i >= 0; i--) {
        p = huge_add(DGU16(0x588c), DGU16(0x588e), (int32_t)i * 2);
        *(uint16_t *)FAR_PTR((uint16_t)(p >> 16), (uint16_t)p) = 0;

        p = huge_add(DGU16(0x588c), DGU16(0x588e), (int32_t)i);
        p = huge_add((uint16_t)p, (uint16_t)(p >> 16), 0x2720);
        *FAR_PTR((uint16_t)(p >> 16), (uint16_t)p) = (uint8_t)i;
    }

    DG16(0x58a0) = 0x101;
    DG16(0x58a4) = 0;
    DG8(0x58ae) = 1;
    DG8(0x58a2) = 0;
    DG16(0x58b2) = 0;
    DG16(0x58b4) = 0;

    p = huge_add(DGU16(0x588c), DGU16(0x588e), 0x3720);
    DG16(0x58aa) = (int16_t)(p >> 16);
    DG16(0x58a8) = (int16_t)p;
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
 * **Measured: the free path is reached on these screens**, so it cannot be
 * verified by exercising only the other branch. It is checked properly now
 * that the runtime's own allocator is transcribed.
 */
void free_if_set(uint16_t p)
{
    if (p != 0)
        io_free(p);
}
/*
 * 0x1c71a
 *
 * Close a resource slot: give back everything it holds and clear its entry in
 * the table at DGROUP 0x57c0. Always answers -1.
 *
 * The record's +0 is a `calloc`ed block and goes through `free_if_set`. Its
 * +2:+4 is a far block from DOS, and that is only freed when there is **no**
 * shared block at DGROUP 0x3576 - when there is, the record was pointed at it
 * rather than given one of its own, and freeing it would take the shared one
 * away.
 *
 * The record itself is freed last, through the same `free_if_set`, and the slot
 * is zeroed whether or not there was anything in it.
 */
int16_t close_resource_slot(uint16_t slot)
{
    uint16_t rec;

    rec = DGU16(0x57c0 + 2 * slot);
    DG16(0x588a) = (int16_t)rec;

    if (rec != 0) {
        free_if_set(DGU16(rec));

        rec = DGU16(0x588a);
        if (!huge_equal(DGU16(rec + 2), DGU16(rec + 4), 0, 0)
            && DGU16(0x3576) == 0 && DGU16(0x3578) == 0)
            dos_free_far(DGU16(rec + 2), DGU16(rec + 4));
    }

    free_if_set(DGU16(0x588a));
    DG16(0x57c0 + 2 * slot) = 0;

    return -1;
}

/*
 * 0x1c783
 *
 * Take a resource slot. Answers its number, or -1 when all hundred are in use
 * or the record cannot be allocated.
 *
 * The table at DGROUP 0x57c0 is a hundred words and a zero means free. The
 * record is 0x21 bytes from `calloc`, so it starts cleared - which matters,
 * because `close_resource_slot` frees whatever pointers it finds in it.
 */
int16_t open_resource_slot(void)
{
    int16_t si;
    uint16_t rec;

    for (si = 0; si < 0x64; si++) {
        if (DGU16(0x57c0 + 2 * si) == 0)
            break;
    }

    if (si == 0x64)
        return -1;

    rec = heap_calloc_far(1, 0x21);
    DG16(0x588a) = (int16_t)rec;
    if (rec == 0)
        return -1;

    DG16(0x57c0 + 2 * si) = (int16_t)rec;
    return si;
}

/*
 * 0x1c7d5
 *
 * Give a slot the working memory its decompression type needs. Answers 0, or -1
 * for a type above 3 or an allocation that failed.
 *
 * The sizes come from the same table the handlers do - fourteen bytes to the
 * entry, based at DGROUP **0x357a**, with the near handler offset six bytes
 * into it. That is where the 0x3580 the dispatcher in 0x1c92b uses comes from.
 *
 * Each entry holds two pairs of sizes and `string_contains_r` on the caller's
 * string chooses between them: a match takes the near size from +0 and the far
 * size from +2, no match takes a default near size of 0x80 and the far size
 * from +4.
 *
 * The near part is `calloc`ed. The far part is only allocated when there is no
 * shared block at DGROUP 0x3576; when there is, the record is pointed at that
 * one instead, which is the arrangement `close_resource_slot` has to know
 * about.
 */
int16_t prepare_resource_slot(int16_t type, uint16_t name)
{
    uint16_t entry;
    uint16_t near_size = 0x80;
    uint16_t far_size;
    uint16_t rec;

    if (type > 3)
        return -1;

    entry = (uint16_t)(0x357a + 14 * type);

    if (string_contains_r(name) != 0) {
        near_size = DGU16(entry);
        far_size = DGU16(entry + 2);
    } else {
        far_size = DGU16(entry + 4);
    }

    rec = DGU16(0x588a);
    DG16(rec) = (int16_t)heap_calloc_far(1, near_size);
    if (DGU16(rec) == 0)
        return -1;

    if (far_size != 0) {
        if (!huge_equal(DGU16(0x3576), DGU16(0x3578), 0, 0)) {
            rec = DGU16(0x588a);
            DG16(rec + 4) = (int16_t)DGU16(0x3578);
            DG16(rec + 2) = (int16_t)DGU16(0x3576);
            DG16(0x588e) = (int16_t)DGU16(0x3578);
            DG16(0x588c) = (int16_t)DGU16(0x3576);
        } else {
            uint32_t p = dos_alloc_bytes(far_size, 0, 0, 0);

            rec = DGU16(0x588a);
            DG16(rec + 4) = (int16_t)(p >> 16);
            DG16(rec + 2) = (int16_t)p;
            DG16(0x588e) = (int16_t)(p >> 16);
            DG16(0x588c) = (int16_t)p;
        }

        rec = DGU16(0x588a);
        if (DGU16(rec + 2) == 0 && DGU16(rec + 4) == 0)
            return -1;
    }

    rec = DGU16(0x588a);
    DG8(rec + 0x20) = (uint8_t)type;
    return 0;
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
 * 0x1d54e
 *
 * Open a resource for reading and answer its slot, or -1.
 *
 * The slot is taken, given the `FILE` it will read from at +6 and that file's
 * current position at +0x1c:+0x1e, and started at offset 5 - past the header
 * this routine is about to consume.
 *
 * `string_contains_r` on the name chooses between two paths, and only one is
 * reached here: the read one, which takes the decompression type from the next
 * byte of the file, gives the slot the memory that type needs, records the
 * caller's size at +0xe:+0x10, reads four more bytes into the record at +0x12,
 * and then calls the type's **reset** through a third pointer in the same
 * fourteen-byte table - at DGROUP 0x3586, which is entry+12.
 *
 * Which resets that dispatch reaches was measured, not read off the table: type
 * 2 to `lzw_reset` and type 3 to `lzss_reset`, and a null entry skips it, which
 * is how types 0 and 1 need nothing done.
 *
 * The other path - the writing one, with its own reset at 0x3584 - is not
 * reached and is left as a stub.
 *
 * The stray `push [bp+0xa]` before the `game_fgetc` is not a leak: the compiler
 * leaves the name on the stack across that call so it can serve as
 * `prepare_resource_slot`'s second argument, and cleans both afterwards.
 */
int16_t open_resource(uint16_t unused, uint16_t file, uint16_t name,
                      uint16_t size_lo, uint16_t size_hi)
{
    int16_t slot;
    uint16_t rec;
    int16_t type;
    int32_t pos;

    (void)unused;

    slot = open_resource_slot();
    if (slot == -1)
        return -1;

    rec = DGU16(0x588a);
    DG16(rec + 6) = (int16_t)file;

    pos = game_ftell(file);
    rec = DGU16(0x588a);
    DG16(rec + 0x1e) = (int16_t)((uint32_t)pos >> 16);
    DG16(rec + 0x1c) = (int16_t)pos;

    rec = DGU16(0x588a);
    DG16(rec + 0xc) = 0;
    DG16(rec + 0xa) = 5;

    if (string_contains_r(name) == 0) {
        not_transcribed("0x1d633, opening a resource for writing");
        return -1;
    }

    type = (int16_t)(game_fgetc(file) & 0xff);
    rec = DGU16(0x588a);
    DG8(rec + 0x20) = (uint8_t)type;

    if (prepare_resource_slot(type, name) == -1) {
        game_fseek(file, 0xffff, 0xffff, 1);
        close_resource_slot((uint16_t)slot);
        return -1;
    }

    rec = DGU16(0x588a);
    DG16(rec + 0x10) = (int16_t)size_hi;
    DG16(rec + 0xe) = (int16_t)size_lo;

    game_fread((uint16_t)(DGU16(0x588a) + 0x12), 1, 4, file);

    {
        uint16_t entry = DGU16(0x3586 + 14 * type);

        if (entry != 0) {
            switch (entry) {
            case 0x0720:                /* image 0x1c970 */
                lzw_reset();
                break;
            case 0x19c5:                /* image 0x1dc15 */
                lzss_reset();
                break;
            default:
                not_transcribed("a reset in the table at DGROUP 0x3586");
                break;
            }
        }
    }

    rec = DGU16(0x588a);
    DG8(rec + 0x20) = (uint8_t)(DG8(rec + 0x20) | 0x40);

    rec = DGU16(0x588a);
    DG8(rec + 0x20) = (uint8_t)(DG8(rec + 0x20) | 0x20);
    return slot;
}

/*
 * 0x1d798
 *
 * Close a resource. Answers what is left at DGROUP 0x589c, which the writing
 * side counts into and the reading side leaves at zero.
 *
 * Bit 0x40 at DGROUP 0x5888 - set when the resource was opened for reading -
 * sends it straight to `close_resource_slot`. Everything else is the writing
 * side: a flush through a second pointer in the fourteen-byte table, at
 * 0x3582, then the four bytes at the record's +0x12 written back over the
 * header. Not reached on these screens, and left as a stub.
 */
int16_t close_resource(int16_t handle)
{
    if (select_resource(handle) == 0)
        return -1;

    DG16(0x589c) = 0;

    if ((DG8(0x5888) & 0x40) == 0) {
        not_transcribed("0x1d7c1, flushing a resource opened for writing");
        return -1;
    }

    close_resource_slot((uint16_t)handle);
    return DG16(0x589c);
}

/*
 * 0x1d868
 *
 * Read a resource into memory. Answers what `resource_read` answered, or -1 if
 * the handle names nothing.
 *
 * Three things happen before the read. The resource is selected, which is what
 * makes DGROUP 0x588a and the rest point at it; the destination far pointer is
 * **normalised** and kept at 0x5894, because the decompressors step it with
 * huge-pointer arithmetic that assumes it is; and bit 0x40 is set at 0x57ba,
 * which is what tells the emitters to write rather than skip.
 */
int16_t read_resource(int16_t handle, uint16_t dst_off, uint16_t dst_seg,
                      uint16_t count)
{
    uint32_t p;

    if (select_resource(handle) == 0)
        return -1;

    p = normalise_far_ptr_far(dst_off, dst_seg);
    DG16(0x5896) = (int16_t)(p >> 16);
    DG16(0x5894) = (int16_t)p;

    DG8(0x57ba) = (uint8_t)(DG8(0x57ba) | 0x40);

    return resource_read((uint16_t)handle, count);
}

/*
 * 0x1d95f
 *
 * The size of a resource, as a far value in DX:AX, or -1 for a handle that
 * names nothing. It is the pair at the record's +0x12:+0x14 - the four bytes
 * `open_resource` read out of the header.
 */
uint32_t resource_size(int16_t handle)
{
    uint16_t rec;

    if (select_resource(handle) == 0)
        return 0xffffffffu;

    rec = DGU16(0x588a);
    return ((uint32_t)DGU16(rec + 0x14) << 16) | DGU16(rec + 0x12);
}

/*
 * 0x1d983
 *
 * Seek within a resource, answering the position reached as a far value in
 * DX:AX, or -1 if the handle names nothing.
 *
 * A compressed stream cannot be seeked, so this **skips by decompressing**.
 * The target is worked out from the whence - 0 from the start, 1 from the
 * position at the record's +0x16:+0x18, 2 from the size at +0x12:+0x14 - and
 * then the difference is read in chunks of at most 0x7d00 bytes and thrown
 * away, which is what `read_resource` not having set bit 0x40 at 0x57ba makes
 * happen.
 *
 * A target already reached returns at once. A target *behind* the position
 * needs the stream restarted through `restart_resource_stream`, after which
 * the position is 0 and the target itself is the distance to skip - nothing on
 * these screens seeks backwards, so that path is transcribed and unexercised.
 *
 * A target past the end is clamped to it, and each chunk re-normalises the
 * source pointer at 0x5898 from the record's own far pointer plus its offset.
 */
uint32_t resource_seek(int16_t handle, uint16_t lo, uint16_t hi,
                       int16_t whence)
{
    uint16_t rec;
    uint16_t t_lo = 0, t_hi = 0;

    if (select_resource(handle) == 0)
        return 0xffffffffu;

    rec = DGU16(0x588a);

    if (whence == 1) {
        t_hi = DGU16(rec + 0x18);
        t_lo = DGU16(rec + 0x16);
    } else if (whence == 2) {
        t_hi = DGU16(rec + 0x14);
        t_lo = DGU16(rec + 0x12);
    }

    t_hi = (uint16_t)(t_hi + hi + ((uint16_t)(t_lo + lo) < t_lo ? 1 : 0));
    t_lo = (uint16_t)(t_lo + lo);

    rec = DGU16(0x588a);
    if (DGU16(rec + 0x18) == t_hi && DGU16(rec + 0x16) == t_lo)
        return ((uint32_t)t_hi << 16) | t_lo;

    if ((int16_t)DGU16(rec + 0x18) > (int16_t)t_hi
        || (DGU16(rec + 0x18) == t_hi && DGU16(rec + 0x16) > t_lo)) {
        /*
         * Backwards. The stream is started over - its answer is not looked at
         * - and the position is then 0, so the target *is* the distance left
         * to skip and needs no subtracting. A target at the start or before it
         * is already reached.
         */
        restart_resource_stream(handle);

        if (!((int16_t)t_hi > 0 || (t_hi == 0 && t_lo > 0)))
            return 0;
    } else if ((int16_t)DGU16(rec + 0x14) > (int16_t)t_hi
        || (DGU16(rec + 0x14) == t_hi && DGU16(rec + 0x12) > t_lo)) {
        uint16_t n_lo = (uint16_t)(t_lo - DGU16(rec + 0x16));

        t_hi = (uint16_t)(t_hi - DGU16(rec + 0x18)
                          - (t_lo < DGU16(rec + 0x16) ? 1 : 0));
        t_lo = n_lo;
    } else {
        uint16_t n_lo = (uint16_t)(DGU16(rec + 0x12) - DGU16(rec + 0x16));

        t_hi = (uint16_t)(DGU16(rec + 0x14) - DGU16(rec + 0x18)
                          - (DGU16(rec + 0x12) < DGU16(rec + 0x16) ? 1 : 0));
        t_lo = n_lo;
    }

    for (;;) {
        uint16_t n;
        int16_t got;

        if ((int16_t)t_hi > 0 || (t_hi == 0 && t_lo >= 0x7d00))
            n = 0x7d00;
        else
            n = t_lo;

        got = resource_read((uint16_t)handle, n);

        if (t_lo < (uint16_t)got)
            t_hi = (uint16_t)(t_hi - 1);
        t_lo = (uint16_t)(t_lo - got);

        if (t_lo == 0 && t_hi == 0)
            break;

        rec = DGU16(0x588a);
        {
            uint32_t p = huge_add(DGU16(rec + 6), DGU16(rec + 8),
                                  (int32_t)(((uint32_t)DGU16(rec + 0xc) << 16)
                                            | DGU16(rec + 0xa)));

            p = normalise_far_ptr_far((uint16_t)p, (uint16_t)(p >> 16));
            DG16(0x589a) = (int16_t)(p >> 16);
            DG16(0x5898) = (int16_t)p;
        }
    }

    rec = DGU16(0x588a);
    return ((uint32_t)DGU16(rec + 0x18) << 16) | DGU16(rec + 0x16);
}

/*
 * 0x1dae6
 *
 * Put a resource stream back to its beginning, so a seek backwards can then
 * skip forwards to where it wants.
 *
 * Only a stream still marked readable at 0x5888 bit 0x40 can be restarted;
 * anything else answers -1. The decompressor for the current type at DGROUP
 * 0x57be is reset through the third pointer of its fourteen-byte entry at
 * 0x3586 - the same dispatch `open_resource` uses to start one - and a null
 * entry means the type needs nothing done.
 *
 * Then the record goes back to where `open_resource` left it: offset 5, past
 * the header. A stream read from a file seeks that file to its own start at
 * +0x1c plus 5; one read from memory rebuilds the cursor at 0x5898 from the
 * record's block at +6 plus 5. Either way the position at +0x16 and the two
 * bytes at +0x1a - whatever the decompressor had part-read - go to zero.
 */
int16_t restart_resource_stream(int16_t handle)
{
    uint16_t rec;

    if (select_resource(handle) == 0 || (DG8(0x5888) & 0x40) == 0)
        return -1;

    {
        uint16_t entry = DGU16(0x3586 + 14 * DG8(0x57be));

        if (entry != 0) {
            switch (entry) {
            case 0x0720:                /* image 0x1c970 */
                lzw_reset();
                break;
            case 0x19c5:                /* image 0x1dc15 */
                lzss_reset();
                break;
            default:
                not_transcribed("a reset in the table at DGROUP 0x3586");
                break;
            }
        }
    }

    rec = DGU16(0x588a);
    DG16(rec + 0x0c) = 0;
    DG16(rec + 0x0a) = 5;

    rec = DGU16(0x588a);
    if (DG8(rec + 0x20) & 0x20) {
        uint32_t at = (((uint32_t)DGU16(rec + 0x1e) << 16)
                       | DGU16(rec + 0x1c)) + 5;

        game_fseek(DGU16(0x57bc), (uint16_t)at, (uint16_t)(at >> 16), 0);
    } else {
        uint32_t p = huge_add(DGU16(rec + 6), DGU16(rec + 8), 5);

        p = normalise_far_ptr_far((uint16_t)p, (uint16_t)(p >> 16));
        DG16(0x589a) = (int16_t)(p >> 16);
        DG16(0x5898) = (int16_t)p;
    }

    rec = DGU16(0x588a);
    DG16(rec + 0x18) = 0;
    DG16(rec + 0x16) = 0;

    rec = DGU16(0x588a);
    DG8(rec + 0x1b) = 0;

    rec = DGU16(0x588a);
    DG8(rec + 0x1a) = 0;

    return 0;
}

/*
 * 0x1dc15
 *
 * Reset the LZSS state for a new stream. Eight instructions: clear the
 * initialised flag at DGROUP 0x5918 so `decompress_lzss` builds its tree and
 * fills its ring on the next call, empty the bit buffer at 0x3600, and point
 * 0x5912 at the record's own block.
 *
 * Answers 0, which is what the dispatch that reaches it expects of all of them.
 */
int16_t lzss_reset(void)
{
    uint16_t rec = DGU16(0x588a);

    DG16(0x5918) = 0;
    DG16(0x3600) = 0;
    DG8(0x3602) = 0;

    DG16(0x5914) = DG16(rec + 4);
    DG16(0x5912) = DG16(rec + 2);

    return 0;
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
 * 0x1e940
 *
 * A thunk into the video driver: `ljmp [0x43ba]`, which is `vm_blit_bitmap`.
 * It jumps rather than calls, so the driver returns to this routine's caller
 * and reads that caller's arguments off the stack unchanged.
 */
void blit_bitmap_thunk(uint16_t hdr, int16_t x, int16_t y, uint16_t mode)
{
    vm_blit_bitmap(hdr, x, y, mode);
}

/*
 * 0x1e944
 *
 * A thunk into the video driver: `ljmp [0x43ca]`, which is VGA:0x271b. Same
 * arrangement as 0x1e940 - it takes three arguments rather than four, because
 * that is what its caller pushed.
 */
void blit_scaled_thunk(uint16_t hdr, int16_t x, int16_t y)
{
    vm_blit_scaled(hdr, x, y);
}

/*
 * 0x1e94c
 *
 * Put the graphics controller back the way the rest of the code expects it,
 * after a routine that changed it to draw. Write mode 2, every bit of the bit
 * mask, every plane of the map mask - the same three registers `vm_blit_bitmap`
 * restores in its epilogue, and the same values.
 *
 * On any adapter but 0x10 it does nothing at all: the whole body is behind that
 * test, and the routine is two `retf`s in a row in the image because the second
 * one is a separate one-byte routine.
 */
void restore_write_mode(void)
{
    if (DG8(0x38b1) != 0x10)
        return;

    io_out16(PORT_GC_INDEX, 0x0205);            /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xff08);            /* bit mask: every bit */
    io_out16(PORT_SEQ_INDEX, 0x0f02);           /* map mask: every plane */
}

/*
 * 0x1ec36
 *
 * Fade a run of palette entries towards a colour, through the driver's vector
 * at DGROUP 0x43ce - which is VGA:0x0f57, read out of a running machine
 * because nothing in the image writes that word.
 *
 * The size is filed at DGROUP 0x4460 and 0x4462 first, in the order the
 * arguments are *not* in - the colour to 0x4460 and the weight to 0x4462 -
 * before both are passed on unchanged.
 */
void fade_palette_run(uint16_t first, uint16_t count, uint16_t colour,
                      uint16_t weight)
{
    DGU16(0x4460) = weight;
    DGU16(0x4462) = colour;

    vm_blend_palette(first, count, colour, (uint8_t)weight);
}

/*
 * 0x1e967
 *
 * Load a palette and keep it. Takes either a resource name or an already-open
 * file record - `file_record_valid` tells the two apart, and a name is opened
 * here and closed again before returning. Answers the far pointer to the block
 * it allocated, and files that pointer in the table at DGROUP 0x3a2e.
 *
 * That table is nine slots of four bytes, offset at 0x3a2e and segment at
 * 0x3a30, searched from 1 for one whose four bytes are zero. When none is free
 * the search ends with the index at 10 and the routine writes a null pointer
 * into the *eleventh* slot and answers null - which is out of the table, and is
 * what the original does.
 *
 * The palette's length and the chunk name are both chosen by the byte at
 * DGROUP 0x38ad, through the word tables at 0x4466 and 0x44a2. If that chunk is
 * not in the file and DGROUP 0x38af is set, it falls back to a "PAL:AMG:"
 * chunk: 32 Amiga colour words, 4 bits per component, each expanded to the
 * VGA's 6 by masking to four bits and shifting up two. That fills 96 bytes of
 * the 768 and the remaining 672 are zeroed, which is where the 256-entry size
 * comes from.
 */
uint32_t load_palette(uint16_t name)
{
    uint16_t fp = dg_enter(0x34a);
    uint16_t amg = fp;                          /* [bp-0x34a], 0x40 bytes */
    uint16_t buf = (uint16_t)(fp + 0x40);       /* [bp-0x30a], 0x300 bytes */

    uint16_t blk_off = 0, blk_seg = 0;          /* [bp-0xa], [bp-8] */
    uint16_t opened;                            /* [bp-2] */
    int16_t di;
    int32_t size;

    DG16(0x4464) = DG16((uint16_t)(0x4466 + 2 * (int16_t)DGS8(0x38ad)));

    di = 1;
    for (;;) {
        if ((DGU16((uint16_t)(0x3a2e + 4 * di))
             | DGU16((uint16_t)(0x3a30 + 4 * di))) == 0)
            break;
        if (di >= 0xa)
            break;
        di++;
    }

    if (di < 0xa) {
        uint32_t chunk;

        if (file_record_valid(name) == 0) {
            opened = 1;
            name = open_file_record(name);
        } else {
            opened = 0;
        }

        chunk = seek_named_chunk(
            name, (uint16_t)DG16((uint16_t)(0x44a2 + 2 * (int16_t)DGS8(0x38ad))),
            0);

        if (chunk != 0xffffffffu) {
            uint32_t blk;

            size = DG16(0x4464);                /* the `cwd` sign-extends it */
            blk = dos_alloc_bytes((uint16_t)size, (uint16_t)(size >> 16), 0, 0);
            blk_off = (uint16_t)blk;
            blk_seg = (uint16_t)(blk >> 16);

            if (blk != 0) {
                game_fread(buf, 1, (uint16_t)DG16(0x4464), name);
                size = DG16(0x4464);
                huge_move(blk_off, blk_seg, buf, DGROUP_SEG,
                          (uint16_t)size, (uint16_t)(size >> 16));
            }
        } else if (DG8(0x38af) != 0) {
            chunk = seek_named_chunk(name, 0x44c6, 0);      /* "PAL:AMG:" */

            if (chunk != 0xffffffffu
                && game_fread(amg, 1, 0x40, name) != 0) {
                uint32_t blk;

                size = DG16(0x4464);
                blk = dos_alloc_bytes((uint16_t)size, (uint16_t)(size >> 16),
                                      0, 0);
                blk_off = (uint16_t)blk;
                blk_seg = (uint16_t)(blk >> 16);

                if (blk != 0) {
                    uint16_t p_seg = blk_seg;   /* [bp-4] */
                    uint16_t p_off = blk_off;   /* [bp-6] */
                    int16_t si;

                    for (si = 0; si < 0x20; si++) {
                        int16_t w = DG16((uint16_t)(amg + si * 2));

                        FAR8(p_seg, p_off++) = (uint8_t)(((w >> 8) & 0xf) << 2);
                        FAR8(p_seg, p_off++) = (uint8_t)(((w >> 4) & 0xf) << 2);
                        FAR8(p_seg, p_off++) = (uint8_t)((w & 0xf) << 2);
                    }
                    for (si = 0; si < 0x2a0; si++)
                        FAR8(p_seg, p_off++) = 0;
                }
            }
        }

        if (opened != 0)
            close_file_record(name);
    }

    DGU16((uint16_t)(0x3a30 + 4 * di)) = blk_seg;
    DGU16((uint16_t)(0x3a2e + 4 * di)) = blk_off;

    dg_leave(0x34a);
    return ((uint32_t)blk_seg << 16) | blk_off;
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
 * 0x20185, body at 0x20189
 *
 * Draw a bitmap in the compressed form `compress_bitmap_list` writes.
 *
 * 0x20185 is a thunk - `ljmp [0x44ea]` - and 0x44ea was measured pointing at
 * the instruction after it, so the vector exists to be repointed and not to
 * reach another module. The port calls the body.
 *
 * **The stream.** A byte of state first, the colour base every pixel is
 * measured from, and then a tag byte per run:
 *
 *   0xc0 | n   n **nibbles**, packed two to a byte, each added to the base.
 *              They are unpacked into a scratch buffer on the stack and blitted
 *              in one call.
 *   0x80 | n   n pixels of one colour, the next byte plus the base.
 *   0x40 | n   move n along the row; n of zero ends the bitmap.
 *   0x00 | n   end of row: step down, and move n back along it. The byte that
 *              follows is peeked at, and if its top two bits are both clear and
 *              its low six are not zero it is consumed as a *further* move of
 *              n << 6 - which is how a skip longer than 63 is written.
 *
 * **Mirroring.** Bit 0 of the mode draws the rows bottom to top, bit 1 draws
 * each row right to left - and then every "move along" is the other way round,
 * including the one at the end of a row.
 *
 * **Clipping.** The byte at 0x3893 turns it on, but the whole bitmap is tested
 * against the window first and the flag turned back off when it fits, so a
 * bitmap wholly on screen pays nothing per run. With it on, each row is tested
 * once - `row_ok` - and each run is trimmed against 0x3894 and 0x3896. A trim
 * of more than 0x3f means the run is entirely outside, because no run is
 * longer than that.
 *
 * The row's base address comes from the table at DGROUP 0x3f82, two bytes per
 * scan line, and is only reloaded when the row changes.
 */
void draw_compressed_bitmap(uint16_t hdr, int16_t x, int16_t y, uint16_t mode)
{
    uint16_t fp = dg_enter(0x158);
    uint16_t scratch = (uint16_t)(fp + 0x000);   /* [bp-0x158] */
    uint16_t vb2 = (uint16_t)(fp + 0x140);       /* [bp-0x18] */
    uint16_t vbase = (uint16_t)(fp + 0x141);     /* [bp-0x17] */
    uint16_t vpage = (uint16_t)(fp + 0x142);     /* [bp-0x16] */
    uint16_t vrow = (uint16_t)(fp + 0x144);      /* [bp-0x14] */
    uint16_t vclip = (uint16_t)(fp + 0x146);     /* [bp-0x12] */
    uint16_t vrowok = (uint16_t)(fp + 0x147);    /* [bp-0x11] */
    uint16_t vp = (uint16_t)(fp + 0x148);        /* [bp-0x10] */
    uint16_t vcut = (uint16_t)(fp + 0x14a);      /* [bp-0x0e] */
    uint16_t vx2 = (uint16_t)(fp + 0x14c);       /* [bp-0x0c] */
    uint16_t vstep = (uint16_t)(fp + 0x14e);     /* [bp-0x0a] */
    uint16_t vskip = (uint16_t)(fp + 0x150);     /* [bp-8] */
    uint16_t vsrc = (uint16_t)(fp + 0x152);      /* [bp-6], offset then segment */
    uint16_t vn = (uint16_t)(fp + 0x156);        /* [bp-2] */
    uint16_t vop = (uint16_t)(fp + 0x157);       /* [bp-1] */

    /*
     * The vector at DGROUP 0x43b6 is the driver's do-nothing stub, so the page
     * comes back exactly as it went in. It is called at all only when 0x3f72
     * is set, and the port keeps the guard so that a build whose 0x3f72 is
     * clear is not silently different.
     */
    DGU16(vpage) = DGU16(0x38a8);
    if (DG16(0x3f72) != 0)
        vm_nothing();

    DG8(vclip) = DG8(0x3893);
    if (DG8(vclip) != 0
        && x >= DG16(0x3894)
        && (int16_t)(x + DG16((uint16_t)(hdr + 6))) <= DG16(0x3896)
        && y >= DG16(0x3898)
        && (int16_t)(y + DG16((uint16_t)(hdr + 8))) <= DG16(0x389a))
        DG8(vclip) = 0;

    if (mode & 1) {
        DG16(vstep) = -1;
        y = (int16_t)(y + DG16((uint16_t)(hdr + 8)) - 1);
    } else {
        DG16(vstep) = 1;
    }

    if (mode & 2)
        x = (int16_t)(x + DG16((uint16_t)(hdr + 6)) - 1);

    if (DG8(vclip) != 0) {
        DG8(vrowok) = (y <= DG16(0x389a) && y >= DG16(0x3898)) ? 1 : 0;
        if (DG8(vrowok) != 0)
            DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
    } else {
        DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
    }

    DGU16((uint16_t)(vsrc + 2)) = DGU16(hdr);              /* the segment */
    DGU16(vsrc) = DGU16((uint16_t)(hdr + 2));              /* the offset */

    DG8(vbase) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
    DGU16(vsrc)++;

    for (;;) {
        DG8(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
        DGU16(vsrc)++;

        if ((DG8(vop) & 0x80) == 0) {
            /* 0x2058b - a move, or the end of a row. */
            if (DG8(vop) & 0x40) {
                DG8(vop) &= 0x3f;
                if (DG8(vop) == 0)
                    break;
                if (mode & 2)
                    x = (int16_t)(x - (int8_t)DG8(vop));
                else
                    x = (int16_t)(x + (int8_t)DG8(vop));
                continue;
            }

            DG8(vop) &= 0x3f;
            y = (int16_t)(y + DG16(vstep));

            if (DG8(vclip) != 0) {
                DG8(vrowok) = (y <= DG16(0x389a) && y >= DG16(0x3898)) ? 1 : 0;
                if (DG8(vrowok) != 0)
                    DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
            } else {
                DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
            }

            if (mode & 2)
                x = (int16_t)(x + (int8_t)DG8(vop));
            else
                x = (int16_t)(x - (int8_t)DG8(vop));

            /*
             * Peek at the next tag without consuming it. Only a tag with both
             * top bits clear is taken here, as a move of its low six bits
             * shifted up by six; anything else is left for the loop to read
             * again.
             */
            DG8(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
            if (((int16_t)(int8_t)DG8(vop) & 0xc0) != 0)
                continue;

            DG16(vskip) = (int16_t)((int8_t)DG8(vop) & 0x3f);
            if (DG16(vskip) == 0)
                continue;

            DGU16(vsrc)++;
            DG16(vskip) = (int16_t)(DG16(vskip) << 6);

            if (mode & 2)
                x = (int16_t)(x + DG16(vskip));
            else
                x = (int16_t)(x - DG16(vskip));
            continue;
        }

        if ((DG8(vop) & 0x40) != 0) {
            /* 0x20272 - a run of nibbles, unpacked into the scratch buffer. */
            DG8(vop) &= 0x3f;
            DG8(vn) = DG8(vop);
            DGU16(vp) = scratch;

            while (DG8(vop) != 0) {
                DG8(vb2) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
                DGU16(vsrc)++;

                DG8(DGU16(vp)) = (uint8_t)(((int16_t)DG8(vb2) >> 4)
                                           + DG8(vbase));
                DGU16(vp)++;
                DG8(vop)--;
                if (DG8(vop) == 0)
                    break;

                DG8(DGU16(vp)) = (uint8_t)((DG8(vb2) & 0x0f) + DG8(vbase));
                DGU16(vp)++;
                DG8(vop)--;
            }

            DGU16(vp) = scratch;

            if (mode & 2) {
                DG16(vx2) = (int16_t)(x - (int8_t)DG8(vn));

                if (DG8(vclip) != 0) {
                    if (DG8(vrowok) == 0)
                        goto advance;

                    while (!(DG16(vx2) >= DG16(0x3894)
                             && x < DG16(0x3896))) {
                        if (DG16(vx2) < DG16(0x3894)) {
                            DG16(vcut) = (int16_t)(DG16(0x3894) - DG16(vx2));
                            if (DG16(vcut) > 0x3f)
                                goto advance;
                            DG8(vn) = (uint8_t)(DG8(vn) - DG8(vcut));
                            if ((int8_t)DG8(vn) <= 0)
                                goto advance;
                            break;
                        }

                        DG16(vcut) = (int16_t)(x - DG16(0x3896));
                        if (DG16(vcut) > 0x3f)
                            goto advance;
                        DG8(vn) = (uint8_t)(DG8(vn) - DG8(vcut));
                        if ((int8_t)DG8(vn) <= 0)
                            goto advance;
                        DGU16(vp) = (uint16_t)(DGU16(vp) + DG16(vcut));
                        x = DG16(0x3896);
                        break;
                    }
                }

                vm_blit_run((uint16_t)x, DG8(vn), dgroup + DGU16(vp),
                            DGU16(vpage), DGU16(vrow), 1);
                goto advance;
            }

            DG16(vx2) = (int16_t)(x + (int8_t)DG8(vn));

            if (DG8(vclip) != 0) {
                if (DG8(vrowok) == 0)
                    goto advance;

                while (!(x >= DG16(0x3894) && DG16(vx2) <= DG16(0x3896))) {
                    if (x < DG16(0x3894)) {
                        DG16(vcut) = (int16_t)(DG16(0x3894) - x);
                        if (DG16(vcut) > 0x3f)
                            goto advance;
                        DG8(vn) = (uint8_t)(DG8(vn) - DG8(vcut));
                        if ((int8_t)DG8(vn) <= 0)
                            goto advance;
                        DGU16(vp) = (uint16_t)(DGU16(vp) + DG16(vcut));
                        x = DG16(0x3894);
                        break;
                    }

                    DG16(vcut) = (int16_t)(DG16(vx2) - DG16(0x3896) - 1);
                    if (DG16(vcut) > 0x3f)
                        goto advance;
                    DG8(vn) = (uint8_t)(DG8(vn) - DG8(vcut));
                    if ((int8_t)DG8(vn) <= 0)
                        goto advance;
                    break;
                }
            }

            vm_blit_run((uint16_t)x, DG8(vn), dgroup + DGU16(vp),
                        DGU16(vpage), DGU16(vrow), 0);
            goto advance;
        }

        /* 0x20429 - a run of one colour. */
        DG8(vop) &= 0x3f;
        DG8(vb2) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
        DGU16(vsrc)++;

        if (mode & 2) {
            DG16(vx2) = (int16_t)(x - (int8_t)DG8(vop));

            if (DG8(vclip) != 0) {
                if (DG8(vrowok) == 0)
                    goto advance;

                while (!(DG16(vx2) >= DG16(0x3894) && x < DG16(0x3896))) {
                    if (DG16(vx2) < DG16(0x3894)) {
                        DG16(vcut) = (int16_t)(DG16(0x3894) - DG16(vx2));
                        if (DG16(vcut) > 0x3f)
                            goto advance;
                        DG8(vop) = (uint8_t)(DG8(vop) - DG8(vcut));
                        if ((int8_t)DG8(vop) <= 0)
                            goto advance;
                        break;
                    }

                    DG16(vcut) = (int16_t)(x - DG16(0x3896));
                    if (DG16(vcut) > 0x3f)
                        goto advance;
                    DG8(vop) = (uint8_t)(DG8(vop) - DG8(vcut));
                    if ((int8_t)DG8(vop) <= 0)
                        goto advance;
                    x = DG16(0x3896);
                    break;
                }
            }

            vm_span((uint16_t)(uint8_t)(DG8(vbase) + DG8(vb2)),
                    (uint16_t)(x - DG8(vop) + 1), DG8(vop),
                    DGU16(vpage), DGU16(vrow));
            goto advance;
        }

        DG16(vx2) = (int16_t)(x + (int8_t)DG8(vop));

        if (DG8(vclip) != 0) {
            if (DG8(vrowok) == 0)
                goto advance;

            while (!(x >= DG16(0x3894) && DG16(vx2) <= DG16(0x3896))) {
                if (x < DG16(0x3894)) {
                    DG16(vcut) = (int16_t)(DG16(0x3894) - x);
                    if (DG16(vcut) > 0x3f)
                        goto advance;
                    DG8(vop) = (uint8_t)(DG8(vop) - DG8(vcut));
                    if ((int8_t)DG8(vop) <= 0)
                        goto advance;
                    x = (int16_t)(x + DG16(vcut));
                    break;
                }

                DG16(vcut) = (int16_t)(DG16(vx2) - DG16(0x3896) - 1);
                if (DG16(vcut) > 0x3f)
                    goto advance;
                DG8(vop) = (uint8_t)(DG8(vop) - DG8(vcut));
                if ((int8_t)DG8(vop) <= 0)
                    goto advance;
                break;
            }
        }

        vm_span((uint16_t)(uint8_t)(DG8(vb2) + DG8(vbase)),
                (uint16_t)x, DG8(vop), DGU16(vpage), DGU16(vrow));

    advance:
        x = DG16(vx2);
    }

    dg_leave(0x158);
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

    /* `cli` / `sti`, around this one instruction and nothing else. */
    io_lock();
    DG16(0x44f7) = (int16_t)(DGU16(0x44f7) | cx);
    io_unlock();

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
 * 0x20767
 *
 * The game's timer interrupt: what everything paced is paced by.
 *
 * It does three things. **DGROUP 0x44ef counts down** - `dec ax / cwd / xor
 * ax,dx`, which is a decrement clamped at zero rather than a wrap, because at
 * -1 the `cwd` makes 0xffff and the `xor` turns -1 into 0. That counter is the
 * intro's frame budget and half the game's timing.
 *
 * Then **sixteen callback slots**: a bitmask at 0x44f7 says which are in use, a
 * counter at 0x4539 and a reload at 0x453b, and a far handler at 0x44f9. A slot
 * whose counter reaches zero calls its handler and reloads. The original writes
 * the sixteen out in full rather than looping - `shr di,1` walks the mask and
 * `ja`/`jae` tell "unused, more to come" from "unused, and that was the last" -
 * and the port folds them into the loop they are.
 *
 * And it **divides itself down**: 0x44f5 counts from 0x44f3, and only when it
 * reaches zero does the old BIOS handler get its tick. So the 8253 is running
 * far faster than 18.2 Hz and the BIOS still sees 18.2.
 *
 * The `mov ax,0x2d3c` that loads DS is a relocation, not a constant.
 */
void timer_tick(void)
{
    uint16_t si = 0;
    uint16_t mask = DGU16(0x44f7);
    int32_t slot;
    int16_t n;

    n = (int16_t)(DG16(0x44ef) - 1);
    if (n < 0)
        n = 0;
    DG16(0x44ef) = n;

    for (slot = 0; slot < 16; slot++) {
        uint16_t used = (uint16_t)(mask & 1);

        mask = (uint16_t)(mask >> 1);

        if (used == 0) {
            if (mask == 0)
                break;
            si = (uint16_t)(si + 4);
            continue;
        }

        {
            int16_t left = (int16_t)(DG16((uint16_t)(0x4539 + si)) - 1);

            if (left == 0) {
                call_timer_handler(DGU16((uint16_t)(0x44f9 + si)),
                                   DGU16((uint16_t)(0x44fb + si)));
                left = DG16((uint16_t)(0x453b + si));
            }
            DG16((uint16_t)(0x4539 + si)) = left;
        }

        si = (uint16_t)(si + 4);
    }

    if (--DG16(0x44f5) != 0) {
        io_out8(0x20, 0x20);            /* end of interrupt */
        return;
    }

    DG16(0x44f5) = DG16(0x44f3);

    /*
     * And chain to the vector `timer_install` displaced, at S1C16(0x446d). That
     * is the BIOS's own handler, which keeps 0040:006c ticking. The port has no
     * BIOS handler to chain to and does not pretend otherwise - nothing here
     * reads the BIOS tick count.
     */
}

/*
 * 0x20838
 *
 * A thunk into the video driver: `ljmp [0x438a]`, which is `vm_blit_rows`.
 */
void blit_rows_thunk(uint16_t src_off, uint16_t src_seg, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_blit_rows(src_off, src_seg, x, y, w, h);
}

/*
 * 0x2083c
 *
 * A thunk into the video driver: `ljmp [0x438e]`, which on this adapter is
 * VGA:0x0252 - the entry that does nothing at all.
 */
void blit_rows_alt_thunk(void)
{
    vm_nothing();
}

/*
 * 0x21088
 *
 * A thunk into the video driver: `ljmp [0x4356]`, which is `vm_copy_rect` -
 * the rectangle copy from one page to the other. Same arrangement as the
 * others: it jumps, so the driver returns to this routine's caller and reads
 * that caller's arguments unchanged.
 */
void copy_rect_thunk(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    vm_copy_rect(x, y, width, height);
}

/*
 * 0x21094
 *
 * Install the game's own keyboard handler, once. DGROUP 0x458c is the flag that
 * says it has been done; a second call skips to the BIOS flag fiddling at the
 * end and answers the flag unchanged.
 *
 * The two vectors it takes over are 09h - the keyboard interrupt - and, when
 * the argument says so, 1Ch, the BIOS timer tick. Both old vectors are kept in
 * **this module's own code segment** at 0x4e3c and 0x4e40, which is why `S1C16`
 * reaches them, and the handlers installed are at 0x4f46 and 0x5136 in the same
 * segment. The `mov ax,0x1c25` that loads DS for the `set vector` call is a
 * relocation, not a constant.
 *
 * Everything between the PCjr test and the flag is for a PCjr, and this is not
 * one: `detect_pcjr` answers 0, `neg ax` leaves carry clear, and the `jae` skips
 * an INT 15h, a look at the BIOS keyboard type at 0040:0096, two bytes patched
 * into the handler at 0x4fd2 and 0x4fde, and four keys remapped in the table at
 * DGROUP 0x468c. Left as an abort rather than guessed at.
 *
 * The tail runs on both paths: Num Lock is cleared in the BIOS shift flags at
 * 0040:0017 and Caps Lock set if DGROUP 0x458d says so. The answer is the
 * install flag in AL - AH is left holding 0x40 from loading ES, which is why
 * only the low byte is worth comparing.
 */
uint16_t install_keyboard(int16_t hook_timer)
{
    if (DG8(0x458c) == 0) {
        uint32_t v;

        v = dos_getvect(0x09);
        S1C16(0x4e3c) = (int16_t)v;
        S1C16(0x4e3e) = (int16_t)(v >> 16);

        v = dos_getvect(0x1c);
        S1C16(0x4e40) = (int16_t)v;
        S1C16(0x4e42) = (int16_t)(v >> 16);

        dos_setvect(0x09, 0x4f46, (uint16_t)(S1C25 >> 4));

        if (hook_timer != 0)
            dos_setvect(0x1c, 0x5136, (uint16_t)(S1C25 >> 4));

        DG8(0x471b) = 0;

        if (detect_pcjr() != 0)
            not_transcribed("0x210f3, the PCjr keyboard path - INT 15h, the "
                            "keyboard type at 0040:0096, and the remapping "
                            "at 0x2110c");

        DG8(0x458c) = 1;
    }

    FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) & 0xdf);

    if (DG8(0x458d) != 0)
        FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) | 0x40);

    return DG8(0x458c);
}

/*
 * 0x21434
 *
 * Take the next key from the **BIOS keyboard buffer**, or answer 0 when there
 * is none. The scancode is the high byte and the character the low one, which
 * is how the caller reads a Tab out of it: `shr ax,8` and compare with 0x0f.
 *
 * This is the ring buffer the keyboard interrupt fills, read directly rather
 * than through INT 16h: the head at 0040:001a, the tail at 0040:001c, and the
 * two words at 0040:0080 and 0040:0082 that say where the ring starts and
 * ends. Head equal to tail is empty. The head advances by two and wraps to the
 * start when it reaches the end.
 *
 * Interrupts are off across the whole of it - `pushf`/`cli` ... `popf` - which
 * is the point of reading the buffer yourself: the handler that fills it must
 * not run between the read of the head and the write of it back. The port has
 * no such handler and nothing to exclude, so the flag save is not transcribed.
 */
uint16_t bios_read_key(void)
{
    uint16_t head = (uint16_t)FAR16(0x40, 0x1a);
    uint16_t tail = (uint16_t)FAR16(0x40, 0x1c);
    uint16_t key;

    if (head == tail)
        return 0;

    key = (uint16_t)FAR16(0x40, head);
    head = (uint16_t)(head + 2);
    if (head == (uint16_t)FAR16(0x40, 0x82))
        head = (uint16_t)FAR16(0x40, 0x80);
    FAR16(0x40, 0x1a) = (int16_t)head;

    return key;
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
 * 0x2149e
 *
 * **Slot 0 of every font array is the current font**, and this is what moves a
 * font in and out of it. The call does two different jobs depending on its
 * argument, which is why it answers a slot number rather than nothing:
 *
 *   `set_font(n)`  copies slot `n` over slot 0 - the five header bytes and the
 *                  three far pointers - and answers `n`. A slot that was never
 *                  loaded is refused and the answer is 0.
 *   `set_font(0)`  changes nothing and instead *asks* which slot the current
 *                  font came from, by looking for one whose far pointer equals
 *                  slot 0's. It answers 0 when slot 0 is empty and 0x14 when
 *                  no slot matches, and the caller has to tell those apart from
 *                  a real slot itself.
 *
 * The search compares the segment first and then the offset, which is why the
 * pointers are read as two words rather than one long.
 */
uint16_t set_font(int16_t slot)
{
    int16_t di = 0;

    if (slot == 0) {
        uint16_t cur_seg = DGU16(0x618c);
        uint16_t cur_off = DGU16(0x618a);
        uint16_t at;

        if ((cur_seg | cur_off) == 0)
            return 0;

        di = 1;
        at = (uint16_t)(0x618a + 4 * di);

        while (di < 0x14) {
            if (DGU16((uint16_t)(at + 2)) == cur_seg
                && DGU16(at) == cur_off)
                break;
            di++;
            at = (uint16_t)(at + 4);
        }

        return (uint16_t)di;
    }

    if (table_618a_in_use(slot) == 0)
        return 0;

    di = slot;

    DG8(0x6176) = DG8((uint16_t)(0x6176 + slot));
    DG8(0x38c4) = DG8((uint16_t)(0x38c4 + slot));
    DG8(0x38d8) = DG8((uint16_t)(0x38d8 + slot));
    DG8(0x627a) = DG8((uint16_t)(0x627a + slot));
    DG8(0x38ec) = DG8((uint16_t)(0x38ec + slot));
    DG8(0x3900) = DG8((uint16_t)(0x3900 + slot));

    DGU16(0x618c) = DGU16((uint16_t)(0x618c + 4 * slot));
    DGU16(0x618a) = DGU16((uint16_t)(0x618a + 4 * slot));
    DGU16(0x61dc) = DGU16((uint16_t)(0x61dc + 4 * slot));
    DGU16(0x61da) = DGU16((uint16_t)(0x61da + 4 * slot));
    DGU16(0x622c) = DGU16((uint16_t)(0x622c + 4 * slot));
    DGU16(0x622a) = DGU16((uint16_t)(0x622a + 4 * slot));

    return (uint16_t)di;
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
 * 0x21f1d
 *
 * Start the mouse, once. The flag at DGROUP 0x48ea says whether it has already
 * been done, and a second call answers 0 without touching anything.
 *
 * INT 33h AX=0 resets the driver and answers 0xffff if one is installed. The
 * original turns that into the flag with `neg ax`, which makes 1 from 0xffff
 * and 0 from 0, and sets carry for any non-zero answer - so the `jae` that
 * follows is "no mouse, give up", and the flag is written either way.
 *
 * With a driver there it sets the cursor far off-screen, shows and immediately
 * hides it, sets the mickeys-per-pixel to 8 by 8, puts the cursor at the
 * origin, limits it to the screen the game recorded at DGROUP 0x3f7a and
 * 0x3f7c, and installs the handler at 0x21fcf for the 0x1f events. None of
 * that is in guest memory.
 *
 * The two bytes it copies at the end are: on adapter 8 - the byte at DGROUP
 * 0x38ad, the same one that chooses the palette length - the cursor's hot spot
 * is taken from a second pair at 0x48e7 and 0x48e9.
 */
uint16_t mouse_init(void)
{
    uint16_t present;

    if (DG8(0x48ea) != 0)
        return 0;

    present = io_mouse_reset();
    DG8(0x48ea) = (uint8_t)(-(int16_t)present);

    if (present == 0)
        return 0;

    io_mouse_move_to(0x7fff, 0x7fff);
    io_mouse_show();
    io_mouse_hide();
    io_mouse_set_speed(8, 8);
    io_mouse_move_to(0, 0);

    mouse_set_ranges(0, 0, DGU16(0x3f7a), DGU16(0x3f7c));

    io_mouse_set_handler(0x1f, 0x5d7f, (uint16_t)(S1C25 >> 4));

    if (DG8(0x38ad) == 8) {
        DG8(0x48e6) = DG8(0x48e7);
        DG8(0x48e8) = DG8(0x48e9);
    }

    return 1;
}

/*
 * 0x21f8d
 *
 * Set how far the cursor may travel, from an origin and a size in cells: INT
 * 33h AX=7 for the horizontal range and AX=8 for the vertical. Both ends are
 * multiplied by four - the same cell-to-pixel scale `mouse_move_to` uses - and
 * the far end has one subtracted before scaling, so a size of `n` cells ends at
 * the last pixel of the `n`th rather than the first pixel of the next.
 *
 * It writes nothing to memory: the ranges live in the mouse driver.
 */
void mouse_set_ranges(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    io_mouse_set_x_range((uint16_t)(x << 2), (uint16_t)((x + w - 1) << 2));
    io_mouse_set_y_range((uint16_t)(y << 2), (uint16_t)((y + h - 1) << 2));
}

/*
 * 0x2200f
 *
 * NEVER REACHED: only `mouse_event` calls this, on the branch that needs a
 * user handler, and nothing installs one. See `mouse_set_user_handler`.
 *
 * Save the VGA state the game's own mouse handler is about to disturb, into
 * DGROUP: graphics controller 0 and 1 at 0x48da, 4 at 0x48dc, 8 at 0x48de,
 * 3 at 0x48e2, and the sequencer's map mask at 0x48e0.
 *
 * Every one is read the same way - index the register, read the data port,
 * keep the old index in `ah` so it can go back - and then set to the value a
 * plain write needs: the set/reset registers to 0, the bit mask to 0xff, the
 * function select to 0, and the map mask to 0xf.
 *
 * The **read-modify-write latch** is the odd part. Between writing register 5
 * and restoring it the routine does `stosb` to `a000:ffff`, and on the way back
 * out reads the same byte. That is not a pixel: writing a byte through the VGA
 * loads the four latches from it, and reading one does the same, so this is how
 * the handler's own read-modify-write state is parked and picked up again. The
 * byte it uses is the very last in the aperture, which no mode this game runs
 * displays.
 *
 * Ours only in that the port has no latch to park: `vga_write`/`vga_read` in
 * io.c model the latches, so the same two accesses do the same thing here.
 */
void mouse_save_vga(void)
{
    uint16_t v;

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)(io_in8(PORT_GC_INDEX) << 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    DG16(0x48da) = (int16_t)v;
    io_out8(PORT_GC_INDEX, 0);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 1);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    DG16(0x48dc) = (int16_t)v;

    io_out8(PORT_GC_INDEX, 5);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, DG8(0x48e8));
    vga_write(0xffff, DG8(0x48e8));          /* park the latches */
    io_out8(PORT_GC_DATA, DG8(0x48e6));

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    DG16(0x48de) = (int16_t)v;
    io_out8(PORT_GC_DATA, 0xff);

    io_out8(PORT_GC_INDEX, 3);
    DG8(0x48e2) = io_in8(PORT_GC_DATA);
    io_out8(PORT_GC_DATA, 0);

    v = (uint16_t)(io_in8(PORT_SEQ_INDEX) << 8);
    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)(v | io_in8(PORT_SEQ_DATA));
    DG16(0x48e0) = (int16_t)v;
    io_out8(PORT_SEQ_DATA, 0x0f);
}

/*
 * 0x22074
 *
 * NEVER REACHED, for the same reason as `mouse_save_vga`.
 *
 * Put back what `mouse_save_vga` took, in the reverse order, index register
 * last so the card is left selecting whatever it was selecting before. The
 * latch byte at `a000:ffff` is read rather than written this time, which
 * reloads the latches from it.
 */
void mouse_restore_vga(void)
{
    uint16_t v;

    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)DG16(0x48e0);
    io_out8(PORT_SEQ_DATA, (uint8_t)v);
    io_out8(PORT_SEQ_INDEX, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 3);
    io_out8(PORT_GC_DATA, DG8(0x48e2));

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)DG16(0x48de);
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 5);
    io_out8(PORT_GC_DATA, DG8(0x48e8));
    (void)vga_read(0xffff);                  /* pick the latches back up */
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)DG16(0x48dc);
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 1);
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)DG16(0x48da);
    io_out8(PORT_GC_DATA, (uint8_t)v);
    io_out8(PORT_GC_INDEX, (uint8_t)(v >> 8));
}

/*
 * 0x21fbe
 *
 * Remember the game's own mouse handler, as a far pointer in DGROUP 0x4744 and
 * 0x4746. `mouse_event` below calls it after it has recorded the event, and
 * calls nothing when both words are zero.
 *
 * **Nothing in the image calls this**, and the only other references to either
 * word are the two inside `mouse_event`. Searched for both, as a far call and
 * as a near one, and as any instruction with those displacements: three sites
 * for 0x4744 and two for 0x4746, all of them here. So the pointer is never
 * set, the branch in `mouse_event` is never taken, and the two routines that
 * save and restore the VGA around it never run. Transcribed because the code
 * is there and the branch has to be right if it is ever reached; marked
 * unreachable because it is.
 */
void mouse_set_user_handler(uint16_t off, uint16_t seg)
{
    DGU16(0x4744) = off;
    DGU16(0x4746) = seg;
}

/*
 * 0x21fcf
 *
 * **The mouse driver's callback.** INT 33h AX=0x0c installs this, and the
 * driver calls it on every event the mask asked for, with the button state in
 * `BL` and the position in `CX` and `DX`.
 *
 * It records all three in DGROUP - the buttons at 0x48eb, which is the byte
 * `flag_bit_48ea` answers from and therefore the *only* way a click reaches
 * the game - and then, if a handler is installed, saves the VGA, calls it, and
 * puts the VGA back.
 *
 * The first thing it does is switch to a stack of its own inside DGROUP, at
 * 0x48d8, with interrupts off across the two writes that change `ss` and `sp`
 * together. That is machine, not program: an interrupt arriving between them
 * would run on half a stack. The port has one stack and no interrupts to
 * arrive, so the switch is **not transcribed** - it is the one part of this
 * routine with nothing to correspond to. Everything it does to memory is here.
 */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y)
{
    DG8(0x48eb) = (uint8_t)buttons;
    DGU16(0x4740) = x;
    DGU16(0x4742) = y;

    if ((DGU16(0x4744) | DGU16(0x4746)) == 0)
        return;

    /*
     * Unreachable: nothing sets 0x4744/0x4746 - see `mouse_set_user_handler`.
     * A far call through a pointer needs a dispatch by offset, the way
     * `call_timer_handler` does it, and there is no caller to learn the
     * offsets from. So this aborts rather than guessing, and rather than
     * quietly skipping the handler and the VGA save around it.
     */
    not_transcribed("0x21ffc, the mouse handler the game never installs");
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
 * 0x221ed
 *
 * Copy `count` bytes between two far pointers, with a **32-bit** count, safe
 * when the two overlap. Answers the destination it was given, unnormalised.
 *
 * The original is this written for an 8086: it normalises both pointers,
 * compares them, and dispatches through a pair of function pointers it stores
 * at `cs:[0x5f99]` and `cs:[0x5f9b]` - the normaliser (0x22161 going up,
 * 0x22173 going down) and the copy loop (0x221d6 up, 0x221bf down, the latter
 * under `std`). It works in chunks of at most 0x7d00 bytes, renormalising at
 * the top of each so an offset can never carry past 64K, and each loop copies
 * one byte first where that aligns the destination and then moves words.
 *
 * **That machinery is not reconstructed.** All of it - the indirect calls, the
 * word moves, the chunking, the alignment step - is there to make the copy fast
 * on a 16-bit machine. The port has a flat address space and none of those
 * costs, and what the two artefacts have to agree on is which bytes end up
 * where, which a direction-aware copy settles.
 *
 * The two pointer words are still written. They are *data* that happens to sit
 * in a code segment, the same as the saved timer vector further up this module,
 * and `S1C16` is how the port reaches that. Nothing reads them back.
 */
uint32_t huge_move(uint16_t dst_off, uint16_t dst_seg,
                   uint16_t src_off, uint16_t src_seg,
                   uint16_t count_lo, uint16_t count_hi)
{
    uint32_t count = ((uint32_t)count_hi << 16) | count_lo;
    uint32_t dst = ((uint32_t)dst_seg << 16) | dst_off;
    uint16_t s_off = src_off, s_seg = src_seg;
    uint16_t d_off = dst_off, d_seg = dst_seg;
    uint32_t src_lin, dst_lin;

    /* The original's dispatch words, stored for the comparison's sake only. */
    /* Stored going up, and overwritten below if the copy has to go down. */
    S1C16(0x5f99) = 0x5f11;
    S1C16(0x5f9b) = 0x5f86;

    normalise_far_ptr(&s_off, &s_seg);
    normalise_far_ptr(&d_off, &d_seg);

    /*
     * The original compares the segments and then the offsets, which after
     * normalisation is a comparison of the two addresses.
     */
    src_lin = ((uint32_t)s_seg << 4) + s_off;
    dst_lin = ((uint32_t)d_seg << 4) + d_off;

    if (src_lin == dst_lin)
        return dst;

    if (src_lin < dst_lin) {
        uint32_t i = count;

        S1C16(0x5f99) = 0x5f23;
        S1C16(0x5f9b) = 0x5f6f;

        while (i-- != 0)
            guest_mem[dst_lin + i] = guest_mem[src_lin + i];
    } else {
        uint32_t i;

        for (i = 0; i < count; i++)
            guest_mem[dst_lin + i] = guest_mem[src_lin + i];
    }

    return dst;
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
 * 0x2307d
 *
 * Load a font into one of the eighteen slots of the table at DGROUP 0x618a,
 * and answer the slot number - or 0 for any failure, which is why the search
 * starts at 2 and not at 0. Like `load_palette` it takes either a resource name
 * or an already-open file record, and closes only what it opened itself.
 *
 * The font's header is a run of single bytes read into parallel arrays indexed
 * by the slot: 0x38c4, 0x38d8, 0x38ec, 0x3900 and, for a compressed font,
 * 0x627a. The **first** byte read is a marker rather than a field, and it picks
 * one of three shapes:
 *
 *   0xfd, 0xff  compressed. 0x6176 gets the marker negated - 3 or 1 - the rest
 *               of the header follows, then a word of decompressed size, and
 *               the body comes through the resource layer (`open_resource`,
 *               `read_resource`, `close_resource`) into a block from DOS. Three
 *               far pointers into that block are filed: the body at 0x61da, and
 *               two more at 0x622a and 0x618a, stepped past 2 and then 1 byte
 *               per glyph of the count at 0x3900.
 *   0xfe        uncompressed, and the byte after the marker is the width in
 *               bytes as it stands.
 *   anything    uncompressed, and the marker *was* the width, in bits: it is
 *               rounded up to whole bytes with `(w + 7) >> 3`.
 *
 * Both uncompressed shapes read the body into one near-heap block and file it
 * at 0x618a with DGROUP as its segment, leaving the other two pointers null.
 *
 * The failure flag at [bp-8] is set once and tested before each further step,
 * which is how the original writes what would now be an early return.
 */
uint16_t load_font(uint16_t name)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t size = (uint16_t)(fp + 0x0a);      /* [bp-4], read into by fread */

    uint16_t di = name;
    uint16_t opened = 0;                        /* [bp-2]  */
    int16_t handle;                             /* [bp-6]  */
    int16_t failed;                             /* [bp-8]  */
    uint16_t blk_seg = 0, blk_off = 0;          /* [bp-0xa], [bp-0xc] */
    uint16_t p;                                 /* [bp-0xe] */
    int16_t si;
    uint16_t bx;

    si = 2;
    for (;;) {
        if ((DGU16((uint16_t)(0x618a + 4 * si))
             | DGU16((uint16_t)(0x618c + 4 * si))) == 0)
            break;
        if (si >= 0x14)
            break;
        si++;
    }

    if (si >= 0x14) {
        dg_leave(0x0e);
        return 0;
    }

    if (file_record_valid(di) == 0) {
        opened = 1;
        di = open_file_record(di);
    } else {
        opened = 0;
    }

    if (seek_named_chunk(di, DGU16(0x495c), 0) == 0xffffffffu) {
        si = 0;
    } else {
        game_fread((uint16_t)(0x38c4 + si), 1, 1, di);

        if (DG8((uint16_t)(0x38c4 + si)) == 0xfd
            || DG8((uint16_t)(0x38c4 + si)) == 0xff) {
            uint32_t r;

            DG8((uint16_t)(0x6176 + si)) =
                (uint8_t)(-(int8_t)DG8((uint16_t)(0x38c4 + si)));

            game_fread((uint16_t)(0x38c4 + si), 1, 1, di);
            game_fread((uint16_t)(0x38d8 + si), 1, 1, di);
            game_fread((uint16_t)(0x627a + si), 1, 1, di);
            game_fread((uint16_t)(0x38ec + si), 1, 1, di);
            game_fread((uint16_t)(0x3900 + si), 1, 1, di);
            game_fread(size, 1, 2, di);

            r = file_record_size(di);
            handle = open_resource(0xffff, di, 0x4963,      /* "r" */
                                   (uint16_t)r, (uint16_t)(r >> 16));
            failed = (handle < 0) ? 1 : 0;

            if (failed == 0)
                failed = ((uint16_t)resource_size(handle) == DGU16(size))
                         ? 0 : 1;

            if (failed == 0) {
                uint32_t blk = dos_alloc_bytes(DGU16(size), 0, 0, 0);

                blk_seg = (uint16_t)(blk >> 16);
                blk_off = (uint16_t)blk;
                failed = (blk == 0) ? 1 : 0;
            }

            if (failed == 0)
                failed = (read_resource(handle, blk_off, blk_seg,
                                        DGU16(size)) == (int16_t)DG16(size))
                         ? 0 : 1;

            if (failed == 0) {
                bx = (uint16_t)(4 * si);

                DGU16((uint16_t)(0x61dc + bx)) = blk_seg;
                DGU16((uint16_t)(0x61da + bx)) = blk_off;

                blk_off = (uint16_t)(blk_off
                                     + 2 * DG8((uint16_t)(0x3900 + si)));

                DGU16((uint16_t)(0x622c + bx)) = blk_seg;
                DGU16((uint16_t)(0x622a + bx)) = blk_off;

                blk_off = (uint16_t)(blk_off + DG8((uint16_t)(0x3900 + si)));

                DGU16((uint16_t)(0x618c + bx)) = blk_seg;
                DGU16((uint16_t)(0x618a + bx)) = blk_off;
            }

            close_resource(handle);

            if (failed != 0) {
                if ((blk_off | blk_seg) != 0)
                    dos_free_far(blk_off, blk_seg);
                si = 0;
            }
        } else {
            int16_t glyph_bytes;

            if (DG8((uint16_t)(0x38c4 + si)) == 0xfe) {
                DG8((uint16_t)(0x6176 + si)) = 2;
                game_fread((uint16_t)(0x38c4 + si), 1, 1, di);
                glyph_bytes = (int16_t)DG8((uint16_t)(0x38c4 + si));
            } else {
                DG8((uint16_t)(0x6176 + si)) = 0;
                glyph_bytes =
                    (int16_t)((int16_t)(DG8((uint16_t)(0x38c4 + si)) + 7) >> 3);
            }
            DG16(size) = glyph_bytes;

            game_fread((uint16_t)(0x38d8 + si), 1, 1, di);
            game_fread((uint16_t)(0x38ec + si), 1, 1, di);
            game_fread((uint16_t)(0x3900 + si), 1, 1, di);

            DG16(size) = (int16_t)(DG16(size)
                * (int16_t)((int16_t)DG8((uint16_t)(0x38d8 + si))
                            * (int16_t)DG8((uint16_t)(0x3900 + si))));

            p = heap_malloc_far(DGU16(size));
            failed = (p == 0) ? 1 : 0;

            if (failed == 0)
                game_fread(p, DGU16(size), 1, di);

            if (failed == 0) {
                bx = (uint16_t)(4 * si);

                DGU16((uint16_t)(0x618c + bx)) = DGROUP_SEG;
                DGU16((uint16_t)(0x618a + bx)) = p;
                DGU16((uint16_t)(0x61dc + bx)) = 0;
                DGU16((uint16_t)(0x61da + bx)) = 0;
                DGU16((uint16_t)(0x622c + bx)) = 0;
                DGU16((uint16_t)(0x622a + bx)) = 0;
            } else {
                if (p != 0)
                    heap_free_far(p);
                si = 0;
            }
        }
    }

    if (opened != 0)
        close_file_record(di);

    dg_leave(0x0e);
    return (uint16_t)si;
}

/*
 * 0x2367c
 *
 * Load a bitmap list. Takes a resource name or an open file record, answers the
 * list it built, and gives every block back on any failure.
 *
 * The shape is: read the header - the number of bitmaps and the list of headers
 * - ask the driver how much room the planar form needs, take that from DOS,
 * read the "BMP:BIN:" chunk into it, and hand the whole thing to
 * `vm_load_bitmap_list` to convert in place. Then, if DGROUP 0x38af is set,
 * look for a "BMP:VGA:" or "BMP:AMG:" chunk and read *that* through the driver
 * as well - 5 for VGA, 6 for Amiga, and the Amiga one is expanded from one bit
 * per pixel to four first, in place and backwards.
 *
 * Three things in it are worth naming:
 *
 *   **A dead branch.** After opening the record it tests `or ax,ax` and then
 *   `jae`, and `or` always clears carry, so the failure jump is never taken. It
 *   is the same slip as the alignment step in `far_memcpy`, and it is
 *   transcribed as it behaves.
 *
 *   **A scratch block that is allocated to be freed.** If DGROUP 0x3576 is null
 *   it asks the near heap for 0x3cc4 bytes, frees them at once, and then asks
 *   for 0x3ac4 - which is how a program of this era makes sure the smaller
 *   block lands at the top of the largest hole. The pointer it keeps is then
 *   pushed up to the next paragraph boundary.
 *
 *   **A retry loop that halves.** The second read's buffer starts at 0x7fff
 *   bytes and the request is halved until DOS can satisfy it, so a machine with
 *   less memory reads in smaller pieces rather than failing.
 *
 * The driver call at vector 0x4382 is `vm_nothing` on this adapter, and nine
 * words are pushed at 0x4382 and 0x437e where five are read.
 */
uint16_t load_bitmap_list(uint16_t name)
{
    uint16_t fp = dg_enter(0x1e);
    uint16_t walk = (uint16_t)(fp + 0x14);      /* [bp-0xa], [bp-8] */
    uint16_t count_at = (uint16_t)(fp + 0x0c);  /* [bp-0x12] */
    uint16_t list_at = (uint16_t)(fp + 0x1c);   /* [bp-2]    */
    uint16_t size_at = (uint16_t)(fp + 0x08);   /* [bp-0x16] */

    uint16_t si = name;
    uint16_t opened = 0;                        /* [bp-0x18] */
    int16_t kind = 0;                           /* [bp-0x1a] */
    uint16_t blk_seg = 0, blk_off = 0;          /* [bp-4], [bp-6]    */
    uint16_t tmp_seg = 0, tmp_off = 0;          /* [bp-0xc], [bp-0xe] */
    uint16_t scratch = 0;                       /* [bp-0x10] */
    uint16_t want_lo, want_hi;                  /* [bp-0x1e], [bp-0x1c] */
    int16_t got;                                /* [bp-0x14] */
    int16_t di = 0;
    uint32_t r;

    DGU16(list_at) = 0;

    if (file_record_valid(si) == 0) {
        opened = 1;
        si = open_file_record(si);
        /* `or ax,ax` then `jae`: the failure jump here is never taken. */
    }

    if (read_bmp_info(si, count_at, list_at) == 0)
        goto done;

    r = vm_bitmap_list_size(DGU16(list_at), size_at);
    want_lo = (uint16_t)r;
    want_hi = (uint16_t)(r >> 16);

    r = dos_alloc_bytes(want_lo, want_hi, 0, 0);
    blk_seg = (uint16_t)(r >> 16);
    blk_off = (uint16_t)r;
    if (r == 0)
        goto done;

    if (DGU16(size_at) != 0) {
        int32_t n = DG16(size_at);              /* the `cwd` sign-extends it */

        r = dos_alloc_bytes((uint16_t)n, (uint16_t)(n >> 16), 0, 0);
        tmp_seg = (uint16_t)(r >> 16);
        tmp_off = (uint16_t)r;
    }

    if ((DGU16(0x3576) | DGU16(0x3578)) == 0) {
        scratch = heap_malloc_far(0x3cc4);
        if (scratch != 0) {
            heap_free_far(scratch);
            scratch = heap_malloc_far(0x3ac4);
            if (scratch != 0) {
                DGU16(0x3578) = DGROUP_SEG;
                DGU16(0x3576) = scratch;
                huge_add_to(0x3576, DGROUP_SEG, 0x10);
                r = normalise_far_ptr_far((uint16_t)(DGU16(0x3576) & 0xfff0),
                                          DGU16(0x3578));
                DGU16(0x3578) = (uint16_t)(r >> 16);
                DGU16(0x3576) = (uint16_t)r;
            }
        }
    }

    if (seek_named_chunk(si, 0x496f, 0) == 0xffffffffu)   /* "BMP:BIN:" */
        goto done;

    r = file_record_size(si);
    di = open_resource(0, si, 0x4978, (uint16_t)r, (uint16_t)(r >> 16));
    if (di < 0)
        goto done;

    DGU16((uint16_t)(walk + 2)) = blk_seg;
    DGU16(walk) = blk_off;

    while (read_resource(di, DGU16(walk), DGU16((uint16_t)(walk + 2)), 0x7fff)
           == 0x7fff)
        huge_add_to(walk, DGROUP_SEG, 0x7fff);

    r = resource_size(di);
    vm_load_bitmap_list(DGU16(list_at), blk_off, blk_seg,
                        (uint16_t)r, (uint16_t)(r >> 16));

    close_resource(di);
    kind = 1;

    if (DG8(0x38af) == 0)
        goto done;

    if (seek_named_chunk(si, 0x497a, 0) != 0xffffffffu)    /* "BMP:VGA:" */
        kind = 5;
    if (seek_named_chunk(si, 0x4983, 0) != 0xffffffffu)    /* "BMP:AMG:" */
        kind = 6;

    if (kind < 5)
        goto done;

    r = file_record_size(si);
    di = open_resource(0, si, 0x498c, (uint16_t)r, (uint16_t)(r >> 16));
    if (di < 0)
        goto done;

    want_hi = 0;
    want_lo = 0x7fff;

    for (;;) {
        r = dos_alloc_bytes(want_lo, want_hi, 0, 0);
        tmp_seg = (uint16_t)(r >> 16);
        tmp_off = (uint16_t)r;
        if (r != 0)
            break;
        /* halve the request, as one 32-bit shift right */
        want_lo = (uint16_t)(((uint32_t)want_hi << 16 | want_lo) >> 1);
        want_hi = (uint16_t)((int16_t)want_hi >> 1);
    }

    DGU16((uint16_t)(walk + 2)) = blk_seg;
    DGU16(walk) = blk_off;

    while ((got = read_resource(di, tmp_off, tmp_seg, want_lo)) > 0) {
        if (kind == 6) {
            expand_1bpp_to_4bpp(tmp_off, tmp_seg, tmp_off, tmp_seg,
                                (uint16_t)got);
            got = (int16_t)(got << 2);
        }

        vm_nothing();       /* vector 0x4382, with five words pushed at it */

        huge_add_to(walk, DGROUP_SEG,
                    (int32_t)(((uint32_t)want_hi << 16 | want_lo) << 1));
    }

    close_resource(di);

done:
    if (huge_equal(tmp_off, tmp_seg, 0, 0) == 0)
        dos_free_far(tmp_off, tmp_seg);

    if (scratch != 0) {
        heap_free_far(scratch);
        DGU16(0x3578) = 0;
        DGU16(0x3576) = 0;
    }

    if (kind == 0) {
        if (huge_equal(blk_off, blk_seg, 0, 0) == 0)
            dos_free_far(blk_off, blk_seg);

        if (di != 0)
            close_resource(di);

        free_bitmap_list(DGU16(list_at));
        DGU16(list_at) = 0;
    }

    if (opened != 0)
        close_file_record(si);

    {
        uint16_t answer = DGU16(list_at);

        dg_leave(0x1e);
        return answer;
    }
}

/*
 * 0x23a18
 *
 * Give back a bitmap list: the block its first word points at, and then the
 * list itself. Both are guarded against null, though only the second guard can
 * ever fire - the caller has already tested the list.
 */
void free_bitmap_list(uint16_t list)
{
    if (DGU16(list) != 0)
        heap_free_far(DGU16(list));

    if (list != 0)
        heap_free_far(list);
}

/*
 * 0x23a3c
 *
 * Give back everything a bitmap list owns: the block its first header points
 * at, and then the list itself through `free_bitmap_list`.
 *
 * The far pointer is read out of the header the way `vm_load_bitmap_list` wrote
 * it - segment at +0 and offset at +2 - and the `cwd` and `adc` around that read
 * are a 32-bit expression the compiler emitted and then had no use for: `cwd`
 * sets DX and the next instruction clears it, and the `adc` adds a carry that
 * `add dx, [di+2]` cannot produce. Transcribed as the two words it reads.
 */
void free_bitmaps(uint16_t list)
{
    if (list == 0)
        return;

    {
        uint16_t hdr = DGU16(list);

        dos_free_far(DGU16((uint16_t)(hdr + 2)), DGU16(hdr));
    }

    free_bitmap_list(list);
}

/*
 * 0x23a6a
 *
 * How many entries a null-terminated list of near pointers has. A null list is
 * zero rather than a fault.
 */
uint16_t count_list_entries(uint16_t list)
{
    uint16_t n = 0;

    if (list == 0)
        return 0;

    while (DGU16((uint16_t)(list + 2 * n)) != 0)
        n++;

    return n;
}

/*
 * 0x23a8a
 *
 * Expand one bit per pixel into four, **backwards**, so the source and the
 * destination may be the same block: a set bit becomes colour 1 and a clear one
 * colour 0. Two source bits share a destination byte - the even bit in the low
 * nibble and the odd one in the high - so `count` source bytes make `count * 4`
 * destination bytes, which is why both pointers are first walked to their last
 * byte and then stepped down.
 *
 * The mask that separates the two cases is `test si, 0xaa`: `si` walks 1, 2, 4
 * ... 0x80, and the bits of 0xaa are the odd positions. The odd one *ors* its
 * 0x10 into the byte the even one wrote and then moves the pointer; the even
 * one *assigns*, which is what clears whatever was in that byte before.
 *
 * The source byte is sign-extended before the test - `cbw` - which cannot
 * matter while `si` stays under 0x100, and is transcribed rather than tidied
 * away.
 *
 * Both far pointers live in the caller's argument slots and are walked in
 * place, so the port needs real DGROUP addresses for them: `huge_add_to` takes
 * the address *of* the pointer.
 */
void expand_1bpp_to_4bpp(uint16_t src_off, uint16_t src_seg,
                         uint16_t dst_off, uint16_t dst_seg, uint16_t count)
{
    uint16_t fp = dg_enter(8);
    uint16_t src = fp;                          /* [bp+6]   */
    uint16_t dst = (uint16_t)(fp + 4);          /* [bp+0xa] */
    int16_t di = (int16_t)count;

    DGU16(src) = src_off;
    DGU16((uint16_t)(src + 2)) = src_seg;
    DGU16(dst) = dst_off;
    DGU16((uint16_t)(dst + 2)) = dst_seg;

    huge_add_to(src, DGROUP_SEG, (uint16_t)(di - 1));
    huge_add_to(dst, DGROUP_SEG, (uint16_t)(di * 4 - 1));

    while (di != 0) {
        int16_t byte;
        int16_t si;

        byte = (int16_t)(int8_t)FAR8(DGU16((uint16_t)(src + 2)), DGU16(src));
        huge_sub_from(src, DGROUP_SEG, 1);

        for (si = 1; (si & 0xff) != 0; si = (int16_t)(si << 1)) {
            uint16_t seg = DGU16((uint16_t)(dst + 2));
            uint16_t off = DGU16(dst);

            if ((si & 0xaa) != 0) {
                FAR8(seg, off) = (uint8_t)(FAR8(seg, off)
                                           | ((si & byte) ? 0x10 : 0x00));
                huge_sub_from(dst, DGROUP_SEG, 1);
            } else {
                FAR8(seg, off) = (uint8_t)((si & byte) ? 0x01 : 0x00);
            }
        }

        di--;
    }

    dg_leave(8);
}

/*
 * 0x23b29
 *
 * Load a screen that is *not* in the quadtree form: read its pixels a band at a
 * time and blit each band to the page as it arrives, so a 320x200 picture never
 * needs a 64 KB buffer.
 *
 * "SCR:DIM:" gives the size if it is there and 320x200 is assumed if it is not.
 * "SCR:BIN:" is the planar body, and it is required. Then, on an adapter that
 * DGROUP 0x38af selects, a second body: "SCR:VGA:" - kind 5 - or "SCR:AMG:" -
 * kind 6, which is expanded from one bit per pixel to four as each band lands.
 *
 * The band buffer is `(w / 2) * 128` bytes if the near heap will give it, and
 * is halved until it will, down to one row. `size / (w / 2)` is how many rows
 * fit in whatever was got, and the last band is short - which is what the
 * `imul` after each band is recomputing.
 *
 * The Amiga body is read at a quarter of the size, because the expansion turns
 * each byte into four.
 *
 * Answers the kind, so the caller can tell which of the three shapes it got.
 */
uint16_t load_screen_plain(uint16_t handle)
{
    uint16_t fp = dg_enter(0x14);
    uint16_t w_at = (uint16_t)(fp + 4);          /* [bp-0x10] */
    uint16_t h_at = (uint16_t)(fp + 2);          /* [bp-0x12] */

    uint16_t opened = 0;                         /* [bp-4]  */
    uint16_t kind = 0;                           /* [bp-6]  */
    int16_t res = 0;                             /* [bp-2]  */
    uint16_t buf = 0, buf_seg = 0;               /* [bp-0xe], [bp-0xc] */
    uint16_t bytes;                              /* [bp-8]  */
    uint16_t half;                               /* [bp-0x14] */
    uint16_t band;                               /* [bp-0xa] */
    int16_t si, di;
    uint32_t r;

    DG16(w_at) = 0x140;
    DG16(h_at) = 0xc8;

    vm_reset_attributes();

    if (file_record_valid(handle) == 0) {
        opened = 1;
        handle = open_file_record(handle);
    }

    if (seek_named_chunk(handle, 0x498e, 0) != 0xffffffffu) {   /* "SCR:DIM:" */
        game_fread(w_at, 1, 2, handle);
        game_fread(h_at, 1, 2, handle);
    }

    if (seek_named_chunk(handle, 0x4997, 0) == 0xffffffffu)     /* "SCR:BIN:" */
        goto close;

    r = file_record_size(handle);
    res = open_resource(0, handle, 0x49a0, (uint16_t)r, (uint16_t)(r >> 16));
    if (res < 0)
        goto close;

    half = (uint16_t)(DG16(w_at) >> 1);
    bytes = (uint16_t)(half << 7);

    do {
        buf = heap_malloc_far(bytes);
        buf_seg = DGROUP_SEG;
        if (buf != 0)
            break;
        bytes = (uint16_t)(bytes >> 1);
    } while (bytes >= half);

    if (buf == 0)
        goto close_resource_only;

    di = 0;
    si = (int16_t)(bytes / half);
    band = bytes;
    if (si > DG16(h_at))
        si = DG16(h_at);

    while (di < DG16(h_at)) {
        read_resource(res, buf, buf_seg, band);
        blit_rows_thunk(buf, buf_seg, 0, di, (int16_t)(half << 1), si);

        di = (int16_t)(di + si);
        if ((int16_t)(di + si) > DG16(h_at)) {
            si = (int16_t)(DG16(h_at) - di);
            band = (uint16_t)(si * half);
        }
    }

    kind = 1;

    if (DG8(0x38af) == 0)
        goto free_buf;

    close_resource(res);

    if (seek_named_chunk(handle, 0x49a2, 0) != 0xffffffffu)     /* "SCR:VGA:" */
        kind = 5;
    else if (seek_named_chunk(handle, 0x49ab, 0) != 0xffffffffu) /* "SCR:AMG:" */
        kind = 6;

    if (kind < 5)
        goto free_buf;

    r = file_record_size(handle);
    res = open_resource(0, handle, 0x49b4, (uint16_t)r, (uint16_t)(r >> 16));
    if (res < 0)
        goto free_buf;

    di = 0;
    si = (int16_t)(bytes / half);
    if (kind == 6)
        bytes = (uint16_t)(bytes >> 2);
    band = bytes;
    if (si > DG16(h_at))
        si = DG16(h_at);

    while (di < DG16(h_at)) {
        read_resource(res, buf, buf_seg, band);

        if (kind == 6)
            expand_1bpp_to_4bpp(buf, buf_seg, buf, buf_seg, band);

        blit_rows_alt_thunk();

        di = (int16_t)(di + si);
        if ((int16_t)(di + si) > DG16(h_at)) {
            si = (int16_t)(DG16(h_at) - di);
            band = (uint16_t)(si * half);
            if (kind == 6)
                band = (uint16_t)(band >> 2);
        }
    }

free_buf:
    heap_free_far(buf);

close_resource_only:
    close_resource(res);

close:
    if (opened != 0)
        close_file_record(handle);

    dg_leave(0x14);
    return kind;
}

/*
 * 0x23df2
 *
 * Find the open-file record with a given handle, or 0.
 *
 * Records of 0x43 bytes at DGROUP 0x6292, with the handle at each one's +0.
 *
 * The search runs **downwards from index 3**, not 4: `si` is loaded with 4 and
 * the loop jumps straight to its test, which decrements before comparing. So
 * the fifth record is never looked at, and the highest matching slot below it
 * wins if two ever held the same handle.
 */
uint16_t find_file_record(uint16_t handle)
{
    int16_t i;

    for (i = 3; i >= 0; i--) {
        uint16_t rec = (uint16_t)(0x6292 + 0x43 * i);

        if (DGU16(rec) == handle)
            return rec;
    }

    return 0;
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

/*
 * 0x242af
 *
 * The size of an open file, as a far value in DX:AX, or -1 for a handle that
 * names nothing. It is the pair at the record's +0x3f:+0x41.
 *
 * A handle of zero is refused before the search, which is what makes zero mean
 * "no file" throughout this layer.
 */
uint32_t file_record_size(uint16_t handle)
{
    uint16_t rec;

    if (handle == 0)
        return 0xffffffffu;

    rec = find_file_record(handle);
    if (rec == 0)
        return 0xffffffffu;

    return ((uint32_t)DGU16(rec + 0x41) << 16) | DGU16(rec + 0x3f);
}

/*
 * 0x24308
 *
 * Whether a handle names an open file: 1 or 0. `find_file_record` answers the
 * record and this throws it away, which is the whole routine.
 */
int16_t file_record_valid(uint16_t handle)
{
    return (int16_t)(find_file_record(handle) != 0);
}

/*
 * 0x242d9
 *
 * Close an open file: clear the record's handle at +0 and close the stream.
 * Answers 1, or 0 for a handle of zero or one that names no record.
 *
 * The record is released by zeroing its +0 alone - `find_file_record` reads
 * nothing else to decide a slot is free - so the rest of the 0x43 bytes are
 * left as they were until the slot is taken again.
 */
int16_t close_file_record(uint16_t handle)
{
    uint16_t rec;

    if (handle == 0)
        return 0;

    rec = find_file_record(handle);
    if (rec == 0)
        return 0;

    DG16(rec) = 0;
    game_fclose(handle);
    return 1;
}

/*
 * 0x23e23
 *
 * Put a file record back to how it starts: all 0x43 bytes cleared **except**
 * the handle at +0 and the 32-bit value at +0x1b:+0x1d, which are saved into
 * locals across the clear and written back - and then the file rewound.
 *
 * Saving those two rather than clearing around them is what makes the routine
 * usable on a record that is being reused as well as one being made.
 */
void reset_file_record(uint16_t rec)
{
    uint16_t handle = DGU16(rec);
    uint16_t keep_lo = DGU16(rec + 0x1b);
    uint16_t keep_hi = DGU16(rec + 0x1d);
    int16_t i;

    for (i = 0; i < 0x43; i++)
        DG8((uint16_t)(rec + i)) = 0;

    DG16(rec + 0x1d) = (int16_t)keep_hi;
    DG16(rec + 0x1b) = (int16_t)keep_lo;
    DG16(rec) = (int16_t)handle;

    game_rewind(handle);
}

/*
 * 0x23f2c
 *
 * Open a file through the resource manager: take a free record, open the file,
 * measure it, and answer the handle. 0 if there is no free record or the file
 * is not there.
 *
 * The size is found by seeking to the end and asking where that is, and stored
 * at +0x1b:+0x1d **with bit 31 set** - `or dx,0x8000` - which is a mark of some
 * kind rather than part of the length; nothing here reads it back.
 *
 * `reset_file_record` then clears the rest of the record and rewinds the file,
 * which is why the seek to the end costs nothing.
 */
uint16_t open_file_record(uint16_t name)
{
    uint16_t rec = find_file_record(0);
    int32_t size;

    if (rec == 0)
        return 0;

    DG16(rec) = (int16_t)game_fopen(name, 0x49b6);
    if (DGU16(rec) == 0)
        return 0;

    game_fseek(DGU16(rec), 0, 0, 2);
    size = game_ftell(DGU16(rec));

    DG16(rec + 0x1d) = (int16_t)(((uint32_t)size >> 16) | 0x8000);
    DG16(rec + 0x1b) = (int16_t)size;

    reset_file_record(rec);
    return DGU16(rec);
}

/*
 * 0x23e70
 *
 * Compare two strings for at most `n` characters, answering 1 if they agree and
 * 0 if they do not.
 *
 * The end test comes **before** the count test, so two strings that both end
 * agree however small `n` is - including zero. Running the count out with both
 * still going also answers 1, which is what makes this a prefix comparison
 * rather than a full one.
 */
int16_t string_equal_upto(uint16_t a, uint16_t b, uint16_t n)
{
    for (;;) {
        if (DG8(a) == 0 && DG8(b) == 0)
            return 1;
        if (n == 0)
            return 1;
        n--;

        if (DG8(a) != DG8(b))
            return 0;
        a++;
        b++;
    }
}

/*
 * 0x23ea8
 *
 * Copy a file record out to the caller: 0x43 bytes from the record with the
 * given handle. Answers the destination, or 0 for a null destination, a null
 * handle, or a handle that names no record.
 */
uint16_t copy_file_record(uint16_t dst, uint16_t handle)
{
    uint16_t rec;

    if (handle == 0 || dst == 0)
        return 0;

    rec = find_file_record(handle);
    if (rec == 0)
        return 0;

    far_move(rec, DGROUP_SEG, dst, DGROUP_SEG, 0x43);
    return dst;
}

/*
 * 0x23f90
 *
 * Put a file record back to the copy saved at DGROUP 0x639e and seek the file
 * to where that copy says it was. Always answers -1.
 *
 * This is `seek_named_chunk`'s failure path: it takes the snapshot on the way
 * in and comes back here whenever the walk cannot go on, so a failed search
 * leaves the record exactly as it found it.
 */
uint32_t restore_file_record(uint16_t rec)
{
    far_move(0x639e, DGROUP_SEG, rec, DGROUP_SEG, 0x43);
    game_fseek(DGU16(rec), DGU16(rec + 0x3b), DGU16(rec + 0x3d), 0);
    return 0xffffffffu;
}

/*
 * 0x23fc2
 *
 * Walk into a file's nested chunks along a path of four-character names, and
 * answer where the wanted one starts - as a far value in DX:AX - or -1.
 *
 * The path is a string of four-character names with no separators, so its
 * length must be a non-zero multiple of four; anything else is refused before
 * the file is touched.
 *
 * The record carries the whole walk: **the depth in bytes** at +0x37, a stack
 * of six chunk-end positions from +0x1b - indexed by that depth, which is why
 * it is a multiple of four - the current position at +0x3b, the current chunk's
 * size at +0x3f, and the path walked so far from +2. Six levels is the limit,
 * and a depth reaching 0x18 gives up.
 *
 * Bit 15 of a stored chunk end says the chunk is a container: with it set the
 * walk descends, reading a four-character name and a four-byte size and pushing
 * the new end; with it clear the chunk is data and is skipped over.
 *
 * The `+0x39` field remembers how many matches a previous call already went
 * past, so asking for a later index continues from there rather than starting
 * again - and asking for an earlier one resets the record and starts over.
 *
 * There is a dead test in the middle: `cmp word [si+0x3f],0` followed by `jb`,
 * which on an unsigned comparison against zero can never be taken. It is
 * transcribed as the nothing it does.
 */
uint32_t seek_named_chunk(uint16_t handle, uint16_t path, int16_t index)
{
    uint16_t si;
    int16_t di = 0;
    int16_t keep;

    if (handle == 0)
        return 0xffffffffu;

    si = find_file_record(handle);
    if (si == 0)
        return 0xffffffffu;

    while (DG8((uint16_t)(path + di)) != 0)
        di++;

    if (di == 0 || (di & 3) != 0)
        return 0xffffffffu;

    far_move(si, DGROUP_SEG, 0x639e, DGROUP_SEG, 0x43);

    if (string_equal_upto(path, (uint16_t)(si + 2), 0x19) != 0) {
        if (index == 0) {
            int32_t pos = game_ftell(DGU16(si));

            if ((uint16_t)((uint32_t)pos >> 16) == DGU16(si + 0x3d)
                && (uint16_t)pos == DGU16(si + 0x3b))
                goto at_position;
        }

        if (index == -1) {
            game_fseek(DGU16(si), DGU16(si + 0x3b), DGU16(si + 0x3d), 0);
            goto at_position;
        }

        if (DG16(si + 0x39) != 0) {
            if (index != 0) {
                keep = index;
                if (DG16(si + 0x39) < index) {
                    index = (int16_t)(index - DG16(si + 0x39));
                } else if (DG16(si + 0x39) == index) {
                    game_fseek(DGU16(si), DGU16(si + 0x3b),
                               DGU16(si + 0x3d), 0);
                    goto at_position;
                } else {
                    reset_file_record(si);
                }
            } else {
                index = 1;
                keep = (int16_t)(DG16(si + 0x39) + 1);
            }
        } else {
            keep = index;
            if (index != 0)
                reset_file_record(si);
            else
                index = 1;
        }
    } else {
        if (index > 0) {
            reset_file_record(si);
            keep = index;
        } else {
            index = 1;
            keep = 0;
        }
    }

    /* 0x240f8 - step over whatever chunk the record is sitting on. */
    {
        uint16_t bx = (uint16_t)(((DG16(si + 0x37) >> 2) << 2) & 0xffff);

        if ((DGU16((uint16_t)(si + bx + 0x1d)) & 0x8000) == 0) {
            uint16_t lo = (uint16_t)(DGU16(si + 0x3b) + DGU16(si + 0x3f));

            DG16(si + 0x3d) = (int16_t)(DGU16(si + 0x3d) + DGU16(si + 0x41)
                                        + (lo < DGU16(si + 0x3b) ? 1 : 0));
            DG16(si + 0x3b) = (int16_t)lo;
        }

        game_fseek(DGU16(si), DGU16(si + 0x3b), DGU16(si + 0x3d), 0);
    }

    for (;;) {
        /* 0x24290 - one match gone by. */
        if (index-- == 0)
            break;

        for (;;) {
            uint16_t bx = (uint16_t)(((DG16(si + 0x37) >> 2) << 2) & 0xffff);

            /* 0x24136 - has this chunk run out? */
            if ((DGU16((uint16_t)(si + bx + 0x1d)) & 0x7fff)
                    == DGU16(si + 0x3d)
                && DGU16((uint16_t)(si + bx + 0x1b)) == DGU16(si + 0x3b)) {
                if (DG16(si + 0x37) == 0)
                    return restore_file_record(si);
                DG16(si + 0x37) = (int16_t)(DG16(si + 0x37) - 4);
                continue;
            }

            if ((DGU16((uint16_t)(si + bx + 0x1d)) & 0x8000) == 0) {
                uint16_t lo = (uint16_t)(DGU16(si + 0x3b) + DGU16(si + 0x3f));

                DG16(si + 0x3d) = (int16_t)(DGU16(si + 0x3d)
                                            + DGU16(si + 0x41)
                                            + (lo < DGU16(si + 0x3b) ? 1 : 0));
                DG16(si + 0x3b) = (int16_t)lo;
                game_fseek(DGU16(si), DGU16(si + 0x3b), DGU16(si + 0x3d), 0);
                continue;
            }

            /* 0x241aa - descend into a container. */
            if (game_fread((uint16_t)(si + DG16(si + 0x37) + 2), 1, 4,
                           DGU16(si)) != 4)
                return restore_file_record(si);

            DG16(si + 0x37) = (int16_t)(DG16(si + 0x37) + 4);
            if (DG16(si + 0x37) >= 0x18)
                return restore_file_record(si);

            DG8((uint16_t)(si + DG16(si + 0x37) + 2)) = 0;

            {
                uint16_t lo = (uint16_t)(DGU16(si + 0x3b) + 8);

                DG16(si + 0x3d) = (int16_t)(DGU16(si + 0x3d)
                                            + (lo < 8 ? 1 : 0));
                DG16(si + 0x3b) = (int16_t)lo;
            }

            if (game_fread((uint16_t)(si + 0x3f), 4, 1, DGU16(si)) != 1)
                return restore_file_record(si);

            {
                uint16_t lo = (uint16_t)(DGU16(si + 0x3b) + DGU16(si + 0x3f));
                uint16_t hi = (uint16_t)(DGU16(si + 0x3d) + DGU16(si + 0x41)
                                         + (lo < DGU16(si + 0x3b) ? 1 : 0));

                bx = (uint16_t)(((DG16(si + 0x37) >> 2) << 2) & 0xffff);
                DG16((uint16_t)(si + bx + 0x1d)) = (int16_t)hi;
                DG16((uint16_t)(si + bx + 0x1b)) = (int16_t)lo;
            }

            DG16(si + 0x41) = (int16_t)(DGU16(si + 0x41) & 0x7fff);

            if (DG16(si + 0x41) < 0)
                return restore_file_record(si);

            {
                uint16_t hi = (uint16_t)(DGU16(si + 0x1d) & 0x7fff);
                uint16_t lo = DGU16(si + 0x1b);

                if (DGU16(si + 0x41) > hi
                    || (DGU16(si + 0x41) == hi && DGU16(si + 0x3f) >= lo))
                    return restore_file_record(si);
            }

            if (DG16(si + 0x37) != di)
                continue;

            if (string_equal_upto((uint16_t)(si + 2), path,
                                  (uint16_t)di) != 0)
                break;
        }
    }

    DG16(si + 0x39) = keep;

at_position:
    return ((uint32_t)DGU16(si + 0x3d) << 16) | DGU16(si + 0x3b);
}

/*
 * 0x20be0
 *
 * Ask whether this is a PCjr, and remember the answer at DGROUP 0x38ac.
 *
 * The test is the ROM: the model byte at F000:FFFE being 0xff and the byte at
 * F000:C000 being 0x21. Answers the flag, sign-extended - and it is only ever
 * **set**, never cleared, so asking twice cannot unset it.
 *
 * Both addresses are ordinary memory as far as the port is concerned: the
 * verifier seeds all of it, ROM included.
 */
int16_t detect_pcjr(void)
{
    if (*FAR_PTR(0xf000, 0xfffe) == 0xff
        && *FAR_PTR(0xf000, 0xc000) == 0x21)
        DG8(0x38ac) = 1;

    return (int16_t)(int8_t)DG8(0x38ac);
}

/*
 * 0x206c1
 *
 * Take over the timer. Answers 1, or 0 if it was already taken or the rate is
 * out of range.
 *
 * The old INT 08h vector is kept **inside this code segment**, at cs:0x446d,
 * not in DGROUP - which is why the port needs `S1C16` to reach it.
 *
 * The divisor is `0xffff / rate`, not the usual 0x1234dc / rate, so the rate is
 * a divisor of the top of a 16-bit counter rather than a frequency in hertz.
 * A rate above 0xff or of zero is refused, and the answer there is 0 - which is
 * `AX` left as the zero it was set to before the range test, not a value
 * written for the purpose.
 *
 * Then the 8253 is programmed - mode 3, low byte then high - the two lowest
 * interrupts unmasked at the PIC, and the handler at cs:0x4517 installed with
 * interrupts off throughout. DGROUP 0x44ee is the flag that says all this has
 * happened.
 */
int16_t timer_install(uint16_t rate)
{
    uint16_t divisor;
    uint32_t v;

    if (DG8(0x44ee) != 0)
        return 0;

    DG16(0x44f7) = 0;
    detect_pcjr();

    v = dos_getvect(8);
    S1C16(0x446d) = (int16_t)v;
    S1C16(0x446f) = (int16_t)(v >> 16);

    if (rate > 0xff || rate == 0)
        return 0;

    DG16(0x44f3) = (int16_t)rate;
    DG16(0x44f5) = (int16_t)rate;

    divisor = (uint16_t)(0xffffu / rate);
    DG16(0x44f1) = (int16_t)divisor;

    /*
     * `cli` from here to just before the flag is set: the 8253 is half
     * programmed and the vector half installed in between, and a tick landing
     * inside that would run through whichever half was in place.
     */
    io_lock();

    io_out8(0x43, 0x36);
    io_out8(0x40, (uint8_t)divisor);
    io_out8(0x40, (uint8_t)(divisor >> 8));
    io_out8(0x21, (uint8_t)(io_in8(0x21) & 0xfc));

    dos_setvect(8, 0x4517, (uint16_t)(S1C25 >> 4));

    io_unlock();                                        /* `sti` */

    DG8(0x44ee) = 1;
    return 1;
}

/*
 * 0x2072e
 *
 * Give the timer back. Answers 1 if it had it, 0 if it did not.
 *
 * The 8253 is put back to a divisor of **zero**, which the chip reads as
 * 0x10000 - the slowest it goes, and the rate DOS expects - and the vector
 * saved at cs:0x446d restored. The two lowest interrupts are unmasked again,
 * which is what `timer_install` did too, so neither routine ever masks them.
 *
 * The answer of 1 is set before the flag at DGROUP 0x44ee is cleared, and the
 * answer of 0 is the `AX` the routine started with rather than one written for
 * the purpose.
 */
int16_t timer_remove(void)
{
    if (DG8(0x44ee) == 0)
        return 0;

    io_out8(0x43, 0x36);
    io_out8(0x40, 0);
    io_out8(0x40, 0);
    io_out8(0x21, (uint8_t)(io_in8(0x21) & 0xfc));

    dos_setvect(8, (uint16_t)S1C16(0x446d), (uint16_t)S1C16(0x446f));

    DG8(0x44ee) = 0;
    return 1;
}

/*
 * 0x2149a
 *
 * A thunk into the video driver: `ljmp [0x4366]`, which is `vm_show_page`.
 *
 * It **jumps** rather than calls, so the driver returns straight to this
 * routine's caller and reads the caller's arguments off the stack unchanged.
 * The port makes it a call, which is the same thing said in C.
 */
void show_page_thunk(uint16_t wait_retrace)
{
    vm_show_page(wait_retrace);
}

/*
 * 0x21ab5
 *
 * A thunk into the video driver: `ljmp [0x435a]`, which is `vm_save_rect`.
 * Same arrangement as 0x2149a.
 */
void save_rect_thunk(uint16_t buf_off, uint16_t buf_seg, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_save_rect(buf_off, buf_seg, x, y, w, h);
}

/*
 * 0x21ab9
 *
 * A thunk into the video driver: `ljmp [0x435e]`, which is `vm_buffer_size`.
 * Same arrangement as 0x2149a.
 */
uint32_t buffer_size_thunk(uint16_t w, uint16_t h)
{
    return vm_buffer_size(w, h);
}

/*
 * 0x2247f
 *
 * A thunk into the video driver: `ljmp [0x4362]`, which is `vm_restore_rect`.
 * Same arrangement as 0x2149a.
 */
void restore_rect_thunk(uint16_t buf_off, uint16_t buf_seg, int16_t x,
                        int16_t y, int16_t w, int16_t h)
{
    vm_restore_rect(buf_off, buf_seg, x, y, w, h);
}

/*
 * 0x22764
 *
 * The BIOS video mode the machine booted in, as bits 4 and 5 of the equipment
 * word at 0040:0010 shifted down - so 0 to 3, of which 3 is monochrome.
 *
 * The port reads that byte out of guest memory. The BIOS data area is at
 * absolute 0x400 and is part of what the verifier seeds and compares, so this
 * needs nothing invented.
 */
uint16_t bios_video_kind(void)
{
    return (uint16_t)((*FAR_PTR(0x40, 0x10) & 0x30) >> 4);
}

/*
 * 0x22113
 *
 * Put the mouse cursor at a given cell: INT 33h AX=4, with the position
 * multiplied by four into DGROUP 0x4740 and 0x4742.
 *
 * The two DGROUP words are written whether or not the driver is there, but only
 * when the flag at 0x48ea says it is - `neg al` sets carry for any non-zero
 * byte, and `jae` skips everything on a zero one. Answers 1 when it moved the
 * cursor and 0 when there was no mouse.
 *
 * The interrupt itself is not reproduced: the port has no mouse driver, and the
 * call leaves nothing in guest memory to compare. What it writes to DGROUP is
 * what anything else can see.
 */
uint16_t mouse_move_to(uint16_t x, uint16_t y)
{
    if (DG8(0x48ea) == 0)
        return 0;

    DG16(0x4740) = (int16_t)(x << 2);
    DG16(0x4742) = (int16_t)(y << 2);

    return 1;
}

/*
 * 0x22190
 *
 * Add a signed 32-bit byte count to a far pointer, the offset in `AX` and
 * segment in `DX`, the count in `CX:BX`. This is the **positive** door; the
 * negative one is at 0x221a4 and normalises afterwards, which this does not.
 *
 * The carry out of the offset add becomes 0x1000 paragraphs on the segment,
 * built without a branch: `sbb bx,bx` makes -1 or 0 and `and bx,0x1000` picks
 * the bit.
 *
 * The high half is folded in with **one `rcr bx,5`** after a `clc`, which is a
 * seventeen-bit rotate: the result is `(cx >> 5) | ((cx & 0xf) << 12)`. The
 * second term is the `cx * 0x1000` the arithmetic wants; the first is a
 * leftover that is zero only while `cx` is under 32, which for a count under
 * two megabytes it is. Transcribed as the rotate it is rather than as the
 * multiply it stands for.
 */
uint32_t huge_add_positive(uint16_t off, uint16_t seg, uint16_t lo,
                           uint16_t hi)
{
    uint32_t sum = (uint32_t)off + lo;

    if (sum > 0xffff)
        seg = (uint16_t)(seg + 0x1000);

    seg = (uint16_t)(seg + ((hi >> 5) | ((hi & 0xf) << 12)));

    return ((uint32_t)seg << 16) | (uint16_t)sum;
}

/*
 * 0x22394
 *
 * Take over INT 0, the divide-by-zero trap. The old vector is kept at DGROUP
 * 0x48ed and the new one points at 0x616e in this code segment - the handler
 * that begins immediately after this routine. DGROUP 0x48ec records that it
 * has been done.
 *
 * The port writes the vector table directly; it is at absolute 0 and is seeded
 * and compared like the rest of memory.
 *
 * The two halves are stored the other way round from how they are read: the
 * offset from 0:0 goes to 0x48ef and the segment from 0:2 to 0x48ed, so the
 * saved pair is segment-first.
 */
void install_divide_trap(void)
{
    DG8(0x48ec) = 1;

    DG16(0x48ef) = (int16_t)*(uint16_t *)(guest_mem + 0);
    DG16(0x48ed) = (int16_t)*(uint16_t *)(guest_mem + 2);

    *(uint16_t *)(guest_mem + 0) = 0x616e;
    *(uint16_t *)(guest_mem + 2) = (uint16_t)(S1C25 >> 4);
}

/*
 * 0x23ee4
 *
 * Copy a file record **in** from the caller: 0x43 bytes over the record whose
 * handle is the first word of what was handed in, and then the file seeked to
 * where the copy says it was.
 *
 * The counterpart of `copy_file_record`, and the pair is how a caller saves and
 * restores a position without the record's own fields moving under it.
 */
int16_t restore_file_record_from(uint16_t src)
{
    uint16_t rec;

    if (src == 0 || DGU16(src) == 0)
        return 0;

    rec = find_file_record(DGU16(src));
    if (rec == 0)
        return 0;

    far_move(src, DGROUP_SEG, rec, DGROUP_SEG, 0x43);
    game_fseek(DGU16(rec), DGU16(rec + 0x3b), DGU16(rec + 0x3d), 0);
    return 1;
}

/*
 * 0x215d5
 *
 * Whether the entry at a given index in the table at DGROUP 0x618a is in use.
 * Answers 1 for a non-null far pointer there, 0 otherwise.
 *
 * The index is refused at both ends - not positive, or 0x14 and over - so the
 * table is twenty entries and index 0 is never accepted, which is what makes 0
 * usable as "no entry".
 */
uint16_t table_618a_in_use(int16_t index)
{
    if (index <= 0 || index >= 0x14)
        return 0;

    if (DGU16((uint16_t)(0x618a + 4 * index)) == 0
        && DGU16((uint16_t)(0x618c + 4 * index)) == 0)
        return 0;

    return 1;
}

/*
 * Which of the two plot routines `draw_char` chose. **The port's own**, and
 * not a transcription: the original keeps a far pointer in a local and calls
 * through it.
 *
 * The original keeps a far pointer in a local and calls through it, so the
 * choice costs nothing per pixel. The port has two named routines instead of a
 * pointer, because a far call through a stack slot has no equivalent here and
 * the two destinations are known: `plot_pixel_clipped` at 0x2244d, and the
 * driver's own plot at the far pointer in DGROUP 0x439e, which is what
 * `plot_pixel_clipped` itself jumps to when nothing is clipped.
 */
static void draw_char_plot(int32_t clipped, int16_t x, int16_t y,
                           int16_t colour)
{
    if (clipped)
        (void)plot_pixel_clipped(x, y, colour);
    else
        (void)vm_plot_pixel(x, y, (uint8_t)colour);
}

/*
 * 0x21670
 *
 * **Draw one character**, and answer how wide it was. Everything the game puts
 * on the screen in words goes through here.
 *
 * *Where the glyph is.* Three font formats, chosen by the marker at DGROUP
 * 0x6176 that `load_font` negated out of the file's first byte:
 *
 *   bit 0 set  proportional. The width is the character's own byte in the
 *              table at 0x622a, and the glyph starts at the offset the word
 *              table at 0x61da holds for it, from the block at 0x618a.
 *   2          fixed, **one byte to a pixel**. The glyph is `index * w * h`
 *              into the block.
 *   otherwise  fixed, **one bit to a pixel**, so a row is `(w + 7) >> 3`
 *              bytes and the glyph is `((w + 7) >> 3) * index * h` in.
 *
 * A character below the font's first code, or at or past its count, draws
 * nothing and answers 0.
 *
 * *Where it goes.* The clip box is tested once for the whole glyph, and the
 * answer picks which routine every pixel then goes through: `plot_pixel_clipped`
 * when any edge is crossed, and the driver's own plot - the far pointer at
 * DGROUP 0x439e - when none is. So a glyph wholly inside the box pays no clip
 * test per pixel, and one that crosses an edge pays it on all of them. The
 * test is `x < left || y < top || x + w > right || y + h > bottom`, and the
 * two width comparisons are **unsigned** where the two origin ones are signed.
 *
 * *The style byte at 0x3892* is five independent things, and they are why this
 * routine is as long as it is:
 *
 *   bit 0  clear means opaque: each row is first drawn as a line in the
 *          background colour at 0x3891 before any pixel of the glyph.
 *   bit 1  bold - every lit pixel is drawn again one to the right.
 *   bit 2  italic - the whole glyph starts `h / 2` to the right and loses one
 *          column every second row, which is a shear done by moving the origin
 *          rather than by transforming anything.
 *   bit 3  underline - on the row the font names at 0x627a, a *blank* pixel is
 *          drawn in the entering colour instead of being skipped.
 *   bit 4  half-tone - a lit pixel is only drawn where `x + y` is odd.
 *
 * In the one-byte-per-pixel format the byte is a colour, not a mask, and a
 * value under 5 is looked up in the table at 0x471e first - which is how the
 * game recolours a font's own shading without touching the glyph.
 *
 * The colour at 0x3890 is saved on the way in and put back on the way out,
 * because the byte-per-pixel path writes it as it goes.
 */
uint16_t draw_char(uint8_t c, int16_t x, int16_t y)
{
    uint8_t  entering = DG8(0x3890);
    int16_t  index    = (int16_t)(c - DG8(0x38ec));
    uint16_t w, h, glyph_seg, glyph_off;
    uint16_t row, col;
    int32_t  clipped;
    uint8_t  mask, pixel;
    int32_t  one_bit;

    if (index < 0)
        return 0;
    if ((int16_t)DG8(0x3900) <= index)
        return 0;

    if (DG8(0x6176) & 1) {
        /*
         * **Both tables are far pointers.** `les bx, [0x622a]` and
         * `les bx, [0x61da]` load a segment as well as an offset, so the width
         * table and the glyph-offset table live in the font's own block and
         * not in DGROUP. Reading the two words as near offsets took the widths
         * and the glyph offsets out of low DGROUP - which drew every character
         * of every proportional string as a block of noise, and is why the
         * briefing's title bar and its description came out smeared while the
         * panel's labels, which are bitmaps, were right.
         */
        w = FAR8(DGU16(0x622c), (uint16_t)(DGU16(0x622a) + index));
        h = DG8(0x38d8);
        glyph_seg = DGU16(0x618c);
        glyph_off = (uint16_t)(DGU16(0x618a)
                               + FARU16(DGU16(0x61dc),
                                        (uint16_t)(DGU16(0x61da)
                                                   + 2 * index)));
    } else {
        uint16_t units;

        w = DG8(0x38c4);
        h = DG8(0x38d8);
        units = (DG8(0x6176) == 2) ? (uint16_t)(index * w)
                                   : (uint16_t)(((w + 7) >> 3) * index);
        glyph_seg = DGU16(0x618c);
        glyph_off = (uint16_t)(DGU16(0x618a) + units * h);
    }

    clipped = (x < DG16(0x3894))
              || (y < DG16(0x3898))
              || ((uint16_t)(x + w) > DGU16(0x3896))
              || ((uint16_t)(y + h) > DGU16(0x389a));

    one_bit = DG8(0x6176) <= 1;

    if (DG8(0x3892) & 4)
        x = (int16_t)(x + h / 2);

    for (row = 0; row < h; row++) {
        if ((DG8(0x3892) & 1) == 0) {
            DG8(0x389e) = DG8(0x3891);
            clip_and_draw_line(x, y, (int16_t)(x + w), y);
        }

        mask = 0x80;
        for (col = 0; col < w; col++) {
            int16_t px;

            if (one_bit) {
                if (mask == 0) {
                    mask = 0x80;
                    glyph_off++;
                }
                pixel = (uint8_t)(FAR8(glyph_seg, glyph_off) & mask);
                mask = (uint8_t)(mask >> 1);
            } else {
                pixel = FAR8(glyph_seg, glyph_off);
                if (pixel != 0)
                    DG8(0x3890) = (pixel < 5)
                                  ? DG8((uint16_t)(0x471e + pixel))
                                  : pixel;
                if ((uint16_t)(w - 1) > col)
                    glyph_off++;
            }

            px = (int16_t)(x + col);

            if (pixel != 0) {
                if ((DG8(0x3892) & 0x10) && (((px + y) & 1) == 0)) {
                    /* half-tone: this one is skipped, but bold still draws */
                    if (DG8(0x3892) & 2)
                        draw_char_plot(clipped, (int16_t)(px + 1), y,
                                       (int16_t)DG8(0x3890));
                } else {
                    draw_char_plot(clipped, px, y, (int16_t)DG8(0x3890));
                    if ((DG8(0x3892) & 0x10) == 0 && (DG8(0x3892) & 2))
                        draw_char_plot(clipped, (int16_t)(px + 1), y,
                                       (int16_t)DG8(0x3890));
                }
            } else if ((DG8(0x3892) & 8) && DG8(0x627a) == row) {
                draw_char_plot(clipped, px, y, (int16_t)entering);
            }
        }

        if ((DG8(0x3892) & 4) && (row & 1))
            x--;

        y++;
        glyph_off++;
    }

    DG8(0x3890) = entering;
    return w;
}

/*
 * 0x218eb
 *
 * **Draw a string.** The body; 0x218d4 below is the door that puts `ds` in
 * front of the caller's near pointer.
 *
 * Two paths, and the whole of the difference is speed. The **slow** one calls
 * `draw_char` for each character and moves x on by what it answers, plus one
 * more when the style says bold. The **fast** one hands the glyph to the
 * driver in registers - `es:si` the pixels, `bx` and `cx` the size, `dx` and
 * `bp` the position - through the far pointer at DGROUP 0x434a, and is only
 * taken when nothing about the drawing is unusual:
 *
 *   the style byte at 0x3892 is 0 or 1 - no bold, italic, underline or
 *   half-tone; the clip flag at 0x3893 is clear; and the font is one of the
 *   two 1-bit formats.
 *
 * Even inside the fast path a character wider than 8 pixels goes back through
 * `draw_char`, because the driver's entry takes a byte a row.
 *
 * A null string - both halves of the pointer zero - draws nothing.
 *
 * The fast path is the driver entry at 0x434a - VGA:0x124b, `vm_blit_glyph` -
 * and it is reached in earnest: `draw_title_bar` turns the clip box off and
 * leaves it off, which is one of the three conditions on its own.
 */
void draw_string_body(uint16_t str, int16_t x, int16_t y)
{
    uint16_t w;

    if (str == 0)
        return;

    /*
     * The three tests are not all the same kind. The style at 0x3892 is
     * compared with `jle` - **signed**, so a style byte with bit 7 set passes
     * it - the clip flag at 0x3893 is sign-extended with `cbw` before being
     * tested against zero, and the font marker at 0x6176 is compared with
     * `jbe`, unsigned. Written as three unsigned tests they would agree on
     * every value this game uses and disagree on a style of 0x80 or more.
     */
    if ((int8_t)DG8(0x3892) <= 1 && (int8_t)DG8(0x3893) == 0
        && DG8(0x6176) <= 1) {
        /*
         * The fast path: a character goes straight to the driver, and one
         * **wider than 8 pixels** falls back to `draw_char`, because the
         * driver's entry takes a byte a row and cannot express more.
         *
         * **The width tested is the previous character's.** `[bp-2]` is seeded
         * with the font's fixed width at 0x38c4 before the loop and the test at
         * the top of each pass reads whatever the last pass left there; only
         * then does the fast branch work out this character's width and store
         * it. For a fixed-width font that makes no difference, and for a
         * proportional one it means a narrow character following a wide one
         * goes to `draw_char` and a wide one following a narrow one goes to the
         * driver - which is how a run of text can be drawn two ways. That is
         * not a reading of the structure; the seed at 0x218f8 is there in the
         * prologue because the first pass has no previous width to use.
         */
        w = DG8(0x38c4);

        while (DG8(str) != 0) {
            int16_t  index;
            uint16_t h, glyph_seg, glyph_off;

            if (w > 8) {
                x = (int16_t)(x + draw_char(DG8(str), x, y));
                str++;
                continue;
            }

            index = (int16_t)(DG8(str) - DG8(0x38ec));

            if ((DGU16(0x61da) | DGU16(0x61dc)) != 0) {
                /* Far pointers, as in `draw_char`; see the note there. */
                w = FAR8(DGU16(0x622c), (uint16_t)(DGU16(0x622a) + index));
                h = DG8(0x38d8);
                glyph_seg = DGU16(0x618c);
                glyph_off = (uint16_t)(DGU16(0x618a)
                                       + FARU16(DGU16(0x61dc),
                                                (uint16_t)(DGU16(0x61da)
                                                           + 2 * index)));
            } else {
                uint16_t stride;

                w = DG8(0x38c4);
                h = DG8(0x38d8);
                stride = (uint16_t)((w + 7) >> 3);
                glyph_seg = DGU16(0x618c);
                glyph_off = (uint16_t)(DGU16(0x618a) + stride * h * index);
            }

            vm_blit_glyph(glyph_seg, glyph_off, w, h, x, y);
            x = (int16_t)(x + w);
            str++;
        }
        return;
    }

    while (DG8(str) != 0) {
        uint16_t w = draw_char(DG8(str), x, y);

        x = (int16_t)(x + w);
        if (DG8(0x3892) & 2)
            x++;
        str++;
    }
}

/*
 * 0x218d4
 *
 * `draw_string_body`, reached the way the game reaches it: the string arrives
 * as a near offset and the body wants a far pointer. Nothing else.
 */
void draw_string(uint16_t str, int16_t x, int16_t y)
{
    draw_string_body(str, x, y);
}

/*
 * 0x21610
 *
 * How wide a string is in the current font. The body; 0x215ff below is the
 * door, and exists only to make a far pointer out of the caller's near one.
 *
 * A character's width comes from one of two places, chosen once before the
 * loop: if the font has a width table - the far pointer at DGROUP 0x61da is
 * not null - each character is looked up in the table at 0x622a, and if it has
 * not, every character is the fixed width at 0x38c4. So a proportional font
 * and a fixed one go through the same loop with the test hoisted out of it.
 *
 * A character is turned into an index by subtracting the font's first code at
 * 0x38ec, and two tests then drop it: a negative index - a character below the
 * font's range - and one at or past the count at 0x3900. Either **stops the
 * measurement**, rather than skipping the character: the `jl` and the `jle`
 * both go to the loop's own test, which then sees the same non-NUL byte and
 * ... does not loop, because the pointer was already advanced. A string with
 * an out-of-range character measures only as far as that character.
 */
uint16_t text_width(uint16_t str)
{
    uint16_t width = 0;
    int16_t  proportional = (DGU16(0x61da) | DGU16(0x61dc)) != 0;

    while (DG8(str) != 0) {
        int16_t index = (int16_t)(DG8(str) - DG8(0x38ec));

        str++;
        if (index < 0)
            break;
        if ((int16_t)DG8(0x3900) <= index)
            break;

        /* `les bx, [0x622a]`: the width table is far. See `draw_char`. */
        width = (uint16_t)(width + (proportional
                                    ? FAR8(DGU16(0x622c),
                                           (uint16_t)(DGU16(0x622a) + index))
                                    : DG8(0x38c4)));
    }

    return width;
}

/*
 * 0x215a5
 *
 * The height of a font's characters, for a font named by slot: the byte at
 * 0x38d8 + slot, which is the same table `load_font` fills.
 *
 * A slot that `table_618a_in_use` says is empty answers 0 - **except slot 0**,
 * which answers its height anyway. The test is `if (!in_use(slot) && slot != 0)
 * return 0`, so the current font is always measurable whether or not it is
 * filed in the table.
 */
uint16_t font_line_height(int16_t slot)
{
    if (table_618a_in_use(slot) == 0 && slot != 0)
        return 0;

    return DG8((uint16_t)(0x38d8 + slot));
}

/*
 * 0x215ff
 *
 * `text_width`, reached the way every caller reaches it: the string arrives as
 * a near offset and the body wants a far pointer, so this pushes `ds` in front
 * of it and calls through. Nothing else.
 */
uint16_t text_width_thunk(uint16_t str)
{
    return text_width(str);
}

/*
 * 0x234d2
 *
 * Read a bitmap's `BMP:INF:` chunk into two allocations: an array of pointers,
 * NUL-terminated, and the ten-byte records it points at. Answers 1, or 0 with
 * everything freed again.
 *
 * The chunk begins with a count, and then two parallel streams of words - a
 * width and a height per record, or so the layout suggests. They are read into
 * one temporary block and threaded into the records afterwards, at +6 and +8.
 *
 * **How many rows are actually there is worked out from the chunk's size**, not
 * taken on trust: the size less the count word has to be at least four bytes
 * per record, and if it is not, only one row is read and every record gets the
 * same pair. That is what the two cursors advancing only when the count matches
 * is doing.
 *
 * The pointer array is `(count + 1) * 2` bytes from `calloc`, so the
 * terminating null is already there before anything is written.
 *
 * Every failure after the first allocation goes through the same cleanup, which
 * frees the records, the array and the temporary in that order.
 */
uint16_t read_bmp_info(uint16_t handle, uint16_t count_at, uint16_t out)
{
    uint16_t tmp = 0;
    uint16_t rows;
    uint16_t di, cursor, a, b;
    int16_t i;

    DG16(out) = 0;

    if (seek_named_chunk(handle, 0x4966, 0) == 0xffffffffu)
        return 0;

    if (game_fread(count_at, 2, 1, handle) != 1)
        return 0;

    DG16(out) = (int16_t)heap_calloc_far((uint16_t)((DGU16(count_at) + 1) * 2),
                                         1);
    if (DGU16(out) == 0)
        goto cleanup;

    DG16(DGU16(out)) = (int16_t)heap_calloc_far(0xa, DGU16(count_at));
    if (DGU16(DGU16(out)) == 0)
        goto cleanup;

    {
        uint32_t sz = file_record_size(handle) - 2;
        uint32_t need = (uint32_t)(int32_t)(int16_t)(DGU16(count_at) * 4);

        rows = (sz >= need) ? DGU16(count_at) : 1;
    }

    tmp = heap_malloc_far((uint16_t)(rows * 4));
    if (tmp == 0)
        goto cleanup;

    if (game_fread(tmp, (uint16_t)(rows * 4), 1, handle) != 1)
        goto cleanup;

    a = tmp;
    b = (uint16_t)(tmp + rows * 2);
    di = DGU16(DGU16(out));
    cursor = DGU16(out);

    for (i = 0; DG16(count_at) > i; i++) {
        DG16(cursor) = (int16_t)di;
        DG16(di + 6) = DG16(a);
        DG16(di + 8) = DG16(b);

        if (DGU16(count_at) == rows) {
            a = (uint16_t)(a + 2);
            b = (uint16_t)(b + 2);
        }

        di = (uint16_t)(di + 0xa);
        cursor = (uint16_t)(cursor + 2);
    }

    DG16(cursor) = 0;
    heap_free_far(tmp);
    return 1;

cleanup:
    if (tmp != 0)
        heap_free_far(tmp);

    if (DGU16(out) != 0) {
        if (DGU16(DGU16(out)) != 0)
            heap_free_far(DGU16(DGU16(out)));
        heap_free_far(DGU16(out));
    }

    return 0;
}

/*
 * 0x225d2
 *
 * Decide which video adapter is there, and answer its code. Hand-written
 * assembly: no frame, the answer in `AL`.
 *
 * DGROUP 0x48f3 is a forced setting, and it is the answer on most paths - the
 * BIOS is asked only to confirm it, not to override it. A zero at DGROUP 0x4344
 * refuses to look at all and answers 0.
 *
 * Of the eight paths only one runs on these screens: 0x48f3 is 0xd, which
 * sends it to the `INT 10h AH=1Ah` display-combination call, and a BL of 7 or
 * 8 there - a monochrome or colour VGA - accepts the forced setting unchanged.
 *
 * The rest are transcribed as stubs, each measured as unreached: the EGA
 * information call at AH=12h, and two probes that write 0x66 to a CRTC register
 * and read it back to tell a real card from an absent one. Those two are the
 * only places in this routine that touch hardware directly, and reproducing
 * them would mean modelling a card that is not there.
 */
uint16_t detect_adapter(void)
{
    uint8_t al = DG8(0x48f3);

    if (DG16(0x4344) == 0)
        return 0;

    if (al == 0)
        goto ask_dcc;

    switch (al) {
    case 9:
        not_transcribed("0x22612, the adapter path for a forced 9");
        return 0;
    case 0xa: case 8: case 0xd: case 0xc: case 0xe: case 0xf:
        goto ask_dcc;
    case 5:
        not_transcribed("0x2264b, the second display-combination path");
        return 0;
    case 2: case 7: case 0xb:
        not_transcribed("0x22682, the EGA information path");
        return 0;
    default:
        not_transcribed("0x226ab, the CRTC probes");
        return 0;
    }

ask_dcc:
    {
        uint16_t bx = io_bios_display_combination();

        if ((bx & 0xff) == 7 || (bx & 0xff) == 8) {
            /* accepted; fall through */
        } else if ((bx >> 8) == 7 || (bx >> 8) == 8) {
            not_transcribed("0x2263a, a second display of 7 or 8");
            return 0;
        } else {
            not_transcribed("0x2264b, reached from the first DCC call");
            return 0;
        }
    }

    al = DG8(0x48f3);
    if (al == 0)
        al = 8;

    return al;
}

/*
 * 0x22efd
 *
 * Load the video driver for an adapter and answer it as a far pointer, or null.
 *
 * The adapter number picks both a **screen size** and a **driver name**, and it
 * does the first through a jump table in this code segment at `cs:0x6e15`,
 * twelve entries covering adapters 4 to 0xf. Anything outside that range takes
 * no default and simply keeps whatever DGROUP 0x3f7a and 0x3f7c already held.
 *
 * The mapping is not one-to-one: several adapters collapse onto driver 0xb or
 * 8, and adapter 4's first assignment of 1 is overwritten by 8 two instructions
 * later without ever being read - dead, and transcribed as such rather than
 * tidied away.
 *
 * The name is then built by copying one of the strings named by the table at
 * DGROUP 0x48ff into the buffer at 0x491d, which is the tail of the chunk path
 * at 0x4919. The chunk is found, its size asked for, a DOS block of that size
 * allocated - freeing whatever was there before - and the driver read into it.
 *
 * The file may arrive as a handle or a name, and one this routine opened is
 * closed again; one it was handed is left alone.
 */
uint32_t load_video_driver(int16_t adapter, uint16_t file)
{
    uint16_t opened = 0;
    uint16_t di;
    int16_t handle;
    uint16_t len_lo, len_hi;
    int16_t si = adapter;

    switch (adapter) {
    case 4:
        si = 1;                           /* overwritten below, never read */
        DG16(0x3f7a) = 0x280;
        si = 8;
        DG16(0x3f7c) = 0x190;
        break;
    case 0xc:
        si = 0xb;
        DG16(0x3f7c) = 0x15e;
        break;
    case 0xd:
        si = 0xb;
        DG16(0x3f7c) = 0x1e0;
        break;
    case 0xe:
        si = 0xb;
        DG16(0x3f7c) = 0x190;
        break;
    case 0xf:
        si = 8;
        DG16(0x3f7c) = 0x190;
        break;
    default:
        break;
    }

    if (file_record_valid(file) == 0) {
        opened = 1;
        di = open_file_record(file);
    } else {
        di = file;
    }

    if (di == 0)
        return 0;

    string_copy_far(0x491d, DGU16((uint16_t)(0x48ff + 2 * si)));

    if (seek_named_chunk(di, 0x4919, 0) == 0xffffffffu)
        return 0;

    {
        uint32_t sz = file_record_size(di);

        handle = open_resource(0xffff, di, 0x495a, (uint16_t)sz,
                               (uint16_t)(sz >> 16));
    }

    if (handle < 0)
        return 0;

    {
        uint32_t sz = resource_size(handle);

        len_lo = (uint16_t)sz;
        len_hi = (uint16_t)(sz >> 16);
    }

    if (!huge_equal(DGU16(0x48f8), DGU16(0x48fa), 0, 0))
        dos_free_far(DGU16(0x48f8), DGU16(0x48fa));

    {
        uint32_t p = dos_alloc_bytes(len_lo, len_hi, 0, 0);

        DG16(0x48fa) = (int16_t)(p >> 16);
        DG16(0x48f8) = (int16_t)p;
    }

    if (huge_equal(DGU16(0x48f8), DGU16(0x48fa), 0, 0))
        return 0;

    read_resource(handle, DGU16(0x48f8), DGU16(0x48fa), len_lo);
    close_resource(handle);

    if (opened != 0)
        close_file_record(di);

    return ((uint32_t)DGU16(0x48fa) << 16) | DGU16(0x48f8);
}

/*
 * 0x22483
 *
 * Bring the video up: pick the adapter, load its driver, start it, and build
 * the far vector table every drawing call goes through. Answers the adapter
 * code, or 0 if there is none or the driver would not load.
 *
 * The table at DGROUP 0x4346 is built in two passes. The driver's start-up
 * answers `DX:SI` pointing at its own table of near offsets; 0x64 words of that
 * are copied in, and then the driver's segment is written into **every second
 * word** 0x32 times. Both counts say fifty entries, which is what the table
 * actually is.
 *
 * DGROUP's own segment is planted at 0000:04f0 on the way past, where anything
 * that needs to find the program's data can read it.
 *
 * **The 8x8 font pointer is not what it looks like.** `INT 10h AX=1130 BH=3`
 * is not implemented by the emulator this port is checked against, so ES and BP
 * come back exactly as they went in - ES zero, BP the frame pointer - and the
 * game stores a "font" that points into its own stack. The port reproduces
 * that, because the emulator is what correct means here; on real hardware the
 * BIOS would answer a real font and these four words would differ. Recorded in
 * STATUS.md as a known divergence from a real machine rather than hidden.
 */
uint16_t vm_init(uint16_t adapter, uint16_t unused, uint16_t file)
{
    uint16_t fp = dg_enter(4);            /* SI and DI; no locals */
    uint16_t bp = (uint16_t)(fp + 4);
    uint16_t al;
    uint16_t r;

    (void)unused;

    DG8(0x48f3) = (uint8_t)adapter;
    DG8(0x3f78) = 0;
    DG8(0x38af) = 0;
    DG16(0x3f7a) = 0x140;
    DG16(0x3f7c) = 0xc8;

    if (DGU16(0x3a2e) != 0 || DGU16(0x3a30) != 0) {
        dos_free_far(DGU16(0x3a2e), DGU16(0x3a30));
        DG16(0x3a2e) = 0;
        DG16(0x3a30) = 0;
    }

    DG8(0x48f2) = (uint8_t)bios_video_kind();

    al = detect_adapter() & 0xff;
    DG8(0x38ad) = (uint8_t)al;

    if (al != 0) {
        uint32_t p = load_video_driver((int16_t)al, file);

        if ((uint16_t)(p >> 16) == 0) {
            DG8(0x38ad) = 0;
        } else {
            uint16_t seg;
            int16_t i;

            DG16(0x48f4) = (int16_t)p;
            DG16(0x48f6) = (int16_t)(p >> 16);

            vm_driver_init(0x3890, 0x4412, DGROUP_SEG);
            seg = DGU16(0x48f6);

            for (i = 0; i < 0x64; i++)
                DG16(0x4346 + 2 * i) =
                    *(int16_t *)FAR_PTR(seg, (uint16_t)(0x13e + 2 * i));

            for (i = 0; i < 0x32; i++)
                DG16(0x4348 + 4 * i) = (int16_t)seg;
        }
    } else {
        DG8(0x38ad) = 0;
    }

    *(uint16_t *)(guest_mem + 0x4f0) = DGROUP_SEG;

    DG16(0x38a6) = DG16(0x38a4);
    DG16(0x38a8) = DG16(0x38a2);

    r = DG8(0x38ad);
    if (r == 0)
        goto out;

    if (DGU16(0x4342) != 0)
        dos_free_far(0, (uint16_t)(DGU16(0x4342) - 1));

    {
        uint32_t p = dos_alloc_bytes((uint16_t)(DGU16(0x3f7c) * 4 + 0x20),
                                     0, 0, 0);

        if ((uint16_t)(p >> 16) == 0)
            goto out;

        DG16(0x4342) = (int16_t)((p >> 16) + 1);
    }

    DG16(0x618a) = (int16_t)bp;           /* the BIOS left BP alone */
    DG16(0x618c) = 0;                     /* and ES was zeroed above */
    DG16(0x618e) = (int16_t)bp;
    DG16(0x6190) = 0;

    DG16(0x38d8) = 0x808;
    DG16(0x38c4) = 0x808;
    DG16(0x38ec) = 0;
    DG16(0x3900) = (int16_t)0xffff;

out:
    dg_leave(4);
    return r;
}
/*
 * 0x24320
 *
 * Planar to chunky, one byte a pixel: the reverse of the driver's
 * `vm_chunky_to_planar`, done in ordinary memory rather than through the card.
 *
 * The four planes are not four pointers but one, `count` bytes apart - the
 * layout `vm_load_bitmap_list` leaves behind - so the routine builds the other
 * three by adding the stride three times and shares their segment. Then for
 * each bit from 0x80 down it gathers that bit out of all four and writes the
 * nibble as a whole byte, which is why the destination is eight times the size
 * of one plane.
 *
 * `count` is decremented when the mask wraps rather than once a pixel, so it
 * counts source bytes and the loop runs eight times for each.
 *
 * A **near** routine: its first argument is at [bp+4], not [bp+6].
 */
void planes_to_chunky(uint16_t dst_off, uint16_t dst_seg,
                      uint16_t src_off, uint16_t src_seg, uint16_t count)
{
    uint16_t p0 = src_off;
    uint16_t p1 = (uint16_t)(src_off + count);
    uint16_t p2 = (uint16_t)(src_off + 2 * count);
    uint16_t p3 = (uint16_t)(src_off + 3 * count);
    uint8_t mask = 0x80;

    while (count != 0) {
        uint8_t v = 0;

        if ((FAR8(src_seg, p0) & mask) != 0) v = (uint8_t)(v | 1);
        if ((FAR8(src_seg, p1) & mask) != 0) v = (uint8_t)(v | 2);
        if ((FAR8(src_seg, p2) & mask) != 0) v = (uint8_t)(v | 4);
        if ((FAR8(src_seg, p3) & mask) != 0) v = (uint8_t)(v | 8);

        FAR8(dst_seg, dst_off) = v;
        dst_off++;

        mask = (uint8_t)(mask >> 1);
        if (mask == 0) {
            count--;
            mask = 0x80;
            p0++;
            p1++;
            p2++;
            p3++;
        }
    }
}

/*
 * 0x243bf
 *
 * Compress a whole list of bitmaps **in place**, over the pixels they came
 * from, and shrink the block down to what the compressed form needed. Answers
 * that size in bytes.
 *
 * Two far pointers run through DGROUP: 0x63e4 is where the output started and
 * does not move, and 0x63ee is where the next byte goes. The second is
 * renormalised at the top of every bitmap - paragraphs carried into the
 * segment, the offset masked to four bits - and the normalised value is what
 * goes back into the header afterwards, so each bitmap's header ends up
 * pointing at its own compressed data. The 0xfffe written to header+4 replaces
 * the mask pointer that `vm_load_bitmap_list` put there; a compressed bitmap
 * carries its transparency in the stream instead.
 *
 * On the adapter that DGROUP 0x38af selects the pixels are already chunky and
 * are compressed where they lie. Anywhere else they are still four planes, so
 * each bitmap is turned chunky into a block from DOS first, compressed out of
 * that, and the block given back - which is why `planes_to_chunky` divides the
 * count by eight: it is told the size in *pixels* and one plane byte is eight
 * of them.
 *
 * The size is measured as the two pointers' difference in segments and bytes
 * and handed to INT 21h AH=4Ah, which is the only place the port has to grow a
 * DOS arena that can shrink a block.
 */
int32_t compress_bitmap_list(uint16_t list, uint16_t colours)
{
    uint16_t si = list;
    uint16_t first = DGU16(list);
    uint16_t segs;
    uint16_t over;

    DG8(0x63f4) = (uint8_t)(colours - 1);
    DGU16(0x63f2) = heap_malloc_far(0x7d0);

    DGU16(0x63e6) = DGU16(first);
    DGU16(0x63e4) = DGU16((uint16_t)(first + 2));
    DGU16(0x63f0) = DGU16(first);
    DGU16(0x63ee) = DGU16((uint16_t)(first + 2));

    while (DGU16(si) != 0) {
        uint16_t hdr = DGU16(si);
        uint16_t at_seg, at_off;
        uint16_t di = DGU16(0x63ee);

        /* Normalise, and remember where this bitmap's own data begins. */
        at_seg = (uint16_t)(DGU16(0x63f0) + (uint16_t)((int16_t)di >> 4));
        at_off = (uint16_t)(di & 0x0f);
        DGU16(0x63f0) = at_seg;
        DGU16(0x63ee) = at_off;

        if (DG8(0x38af) == 0) {
            uint16_t pixels = (uint16_t)(DG16((uint16_t)(hdr + 6))
                                         * DG16((uint16_t)(hdr + 8)));
            uint32_t blk = dos_alloc_bytes(pixels, 0, 0, 0);
            uint16_t blk_seg = (uint16_t)(blk >> 16);
            uint16_t blk_off = (uint16_t)blk;

            pixels = (uint16_t)(pixels >> 3);

            planes_to_chunky(blk_off, blk_seg,
                             DGU16((uint16_t)(hdr + 2)), DGU16(hdr), pixels);

            DGU16(hdr) = blk_seg;
            DGU16((uint16_t)(hdr + 2)) = blk_off;

            compress_bitmap(si);

            dos_free_far(blk_off, blk_seg);
        } else {
            compress_bitmap(si);
        }

        hdr = DGU16(si);
        DGU16(hdr) = at_seg;
        DGU16((uint16_t)(hdr + 2)) = at_off;
        DGU16((uint16_t)(hdr + 4)) = 0xfffe;

        si = (uint16_t)(si + 2);
    }

    segs = (uint16_t)(DGU16(0x63f0) - DGU16(0x63e6));
    over = (uint16_t)(DGU16(0x63ee) - DGU16(0x63e4));
    DGU16(0x63e8) = (uint16_t)(segs + (uint16_t)((int16_t)(over + 0x0f) >> 4));

    io_dos_resize(DGU16(DGU16(list)), DGU16(0x63e8));

    heap_free_far(DGU16(0x63f2));

    return (int32_t)(int16_t)((uint16_t)(segs << 4) + over);
}

/*
 * 0x2451f
 *
 * Emit one value into the compressed bitmap being written at the far pointer in
 * DGROUP 0x63ee, flushing whatever run is pending at DGROUP 0x63e2 first.
 *
 * There are two shapes and they do not meet. With a run pending and a
 * **negative** value, the value is negated and written as its low six bits and
 * then bits 6 to 8, or a zero byte if those are empty, and the rest of the run
 * is padded with zeroes. With a run pending and a value that is not negative,
 * the whole run becomes zero bytes and the count is cleared. Only the second
 * falls through to the tail.
 *
 * The tail writes 0x7f for every 0x3f the value is over, and then the remainder
 * with 0x40 set - a run-length byte and a literal, which is what makes 0x7f the
 * longest run this format can say in one byte.
 *
 * A **near** routine: its argument is at [bp+4].
 */
void emit_packed_value(int16_t value)
{
    int16_t dx = value;

    if (DGU16(0x63e2) != 0) {
        if (dx < 0) {
            dx = (int16_t)(-dx);

            FAR8(DGU16(0x63f0), DGU16(0x63ee)) = (uint8_t)(dx & 0x3f);
            DGU16(0x63ee)++;

            dx = (int16_t)((dx & 0x1c0) >> 6);

            if (dx != 0) {
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = (uint8_t)(dx & 0x3f);
                DGU16(0x63ee)++;
            }

            while (--DGU16(0x63e2) != 0) {
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = 0;
                DGU16(0x63ee)++;
            }
            return;
        }

        while (DGU16(0x63e2)-- != 0) {
            FAR8(DGU16(0x63f0), DGU16(0x63ee)) = 0;
            DGU16(0x63ee)++;
        }
        DGU16(0x63e2) = 0;
    }

    while (dx > 0x3f) {
        FAR8(DGU16(0x63f0), DGU16(0x63ee)) = 0x7f;
        DGU16(0x63ee)++;
        dx = (int16_t)(dx - 0x3f);
    }

    FAR8(DGU16(0x63f0), DGU16(0x63ee)) = (uint8_t)(0x40 | (dx & 0xff));
    DGU16(0x63ee)++;
}

/*
 * 0x245b9
 *
 * Write a run of literal pixels into the compressed bitmap at DGROUP 0x63ee: a
 * marker byte of the count with 0xc0 set, and then the pixels themselves.
 *
 * How they are written depends on DGROUP 0x63f4, which `0x243bf` sets to one
 * less than the number of colours - 0x0f for sixteen. At sixteen colours two
 * pixels share a byte, high nibble first, so an odd count is rounded up and the
 * pad pixel is zeroed in the *source* buffer first, before the marker's count
 * is incremented. Otherwise a pixel is a byte and they go out unchanged.
 *
 * The count is a byte and is compared zero-extended, so a run is at most 255
 * pixels. A **near** routine: its arguments are at [bp+4] and [bp+6].
 */
void write_literal_run(uint8_t count, uint16_t buf)
{
    uint8_t dl = count;
    int16_t si;

    FAR8(DGU16(0x63f0), DGU16(0x63ee)) = (uint8_t)(dl | 0xc0);
    DGU16(0x63ee)++;

    if ((dl & 1) != 0) {
        DG8((uint16_t)(buf + dl)) = 0;
        dl++;
    }

    if (DG8(0x63f4) == 0x0f) {
        for (si = 0; (int16_t)dl > si; si += 2) {
            uint8_t v = (uint8_t)((DG8((uint16_t)(buf + si)) << 4)
                                  | DG8((uint16_t)(buf + si + 1)));

            FAR8(DGU16(0x63f0), DGU16(0x63ee)) = v;
            DGU16(0x63ee)++;
        }
    } else {
        for (si = 0; (int16_t)dl > si; si++) {
            FAR8(DGU16(0x63f0), DGU16(0x63ee)) = DG8((uint16_t)(buf + si));
            DGU16(0x63ee)++;
        }
    }
}

/*
 * 0x24639
 *
 * Compress one row of chunky pixels into the bitmap being written at DGROUP
 * 0x63ee. This is the other half of the format `emit_packed_value` and
 * `emit_literal_run` write bytes for, and between them the three tags are:
 *
 *   0x40 | n   a value, written by `emit_packed_value`
 *   0x80 | n   `n` copies of the byte that follows - a run
 *   0xc0 | n   `n` literal pixels, packed two to a byte at sixteen colours
 *
 * so a count never exceeds 0x3f and a longer run goes out as repeated 0xbf
 * pairs with 0x3f each.
 *
 * The decision is one threshold: DGROUP 0x49ba is the shortest run worth
 * encoding as one, and anything shorter is added to a literal buffer instead.
 * That buffer is flushed when it reaches 0x3f, when a run interrupts it, and
 * at the end of the row.
 *
 * A **near** routine, and it walks its own `remaining` argument down - which
 * nothing can see, because the caller pops it.
 */
void compress_row(uint16_t src, int16_t remaining)
{
    uint16_t fp = dg_enter(0x104);
    uint16_t buf = fp;                  /* [bp-0x104], 0x101 bytes */

    uint16_t di = src;
    uint8_t literals = 0;               /* [bp-3] */
    uint8_t run = 0;                    /* [bp-2] */
    uint8_t value = 0;                  /* [bp-1] */

    while (remaining > 0) {
        uint16_t si = di;

        run = 1;
        value = DG8(si);
        si++;
        while (DG8(si) == value) {
            si++;
            run++;
        }

        if ((int16_t)run >= DG16(0x49ba)) {
            if ((int16_t)run > remaining)
                run = (uint8_t)remaining;

            if (literals != 0) {
                write_literal_run(literals, buf);
                literals = 0;
            }

            remaining = (int16_t)(remaining - run);
            di = (uint16_t)(di + run);

            while (run > 0x3f) {
                run = (uint8_t)(run + 0xc1);        /* less 0x3f */
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = 0xbf;
                DGU16(0x63ee)++;
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = value;
                DGU16(0x63ee)++;
            }

            if (run != 0) {
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = (uint8_t)(0x80 | run);
                DGU16(0x63ee)++;
                FAR8(DGU16(0x63f0), DGU16(0x63ee)) = value;
                DGU16(0x63ee)++;
            }
            run = 0;
        } else {
            remaining--;
            DG8((uint16_t)(buf + literals)) = value;
            literals++;
            di++;
        }

        if (literals == 0x3f) {
            write_literal_run(literals, buf);
            literals = 0;
        }
    }

    if (literals != 0)
        write_literal_run(literals, buf);

    dg_leave(0x104);
}

/*
 * 0x24757
 *
 * Compress one bitmap, row by row, into the stream at DGROUP 0x63ee. This is
 * what drives `compress_row`, `write_literal_run` and `emit_packed_value`; the
 * header is at [si], its pixels at [si+2] with the segment at [si], and its
 * width and height at [si+6] and [si+8].
 *
 * Two things happen before any pixel is written.
 *
 * It reserves **one byte** at the front of the stream and fills it in last:
 * the smallest non-zero pixel value in the whole bitmap. At sixteen colours on
 * an adapter that wants it, that means a first pass over every pixel to find
 * it, and every pixel written afterwards has it subtracted - so a bitmap that
 * uses colours 8 to 15 is stored as 0 to 7 and the byte says where it started.
 * Anywhere else the byte is 1 and the subtraction is a no-op.
 *
 * And DGROUP 0x63e2 counts *rows*, not pixels: it is incremented once a row and
 * flushed as zero bytes when a row turns out to have content. Together with
 * 0x63e6, which counts transparent pixels forward and then has the row width
 * subtracted from it, that is how a run of blank rows costs almost nothing.
 * The count going negative is not a fault - it is the signal
 * `emit_packed_value` reads to tell "so many transparent" from "so many rows".
 *
 * A **near** routine.
 */
void compress_bitmap(uint16_t header)
{
    uint16_t fp = dg_enter(0x14e);
    uint16_t rowbuf = fp;               /* [bp-0x14e] */

    uint16_t si = header;
    uint16_t di = 0;                    /* pixels waiting in the row buffer */
    int16_t blanks = 0;                 /* [bp-6], and it does go negative */
    uint8_t least = 0xff;               /* [bp-7] */
    uint16_t hdr_off, hdr_seg;
    int16_t x, y;

    DGU16(0x63e2) = 0;
    DGU16(0x63e8) = 0;

    DGU16(0x63ec) = DGU16(si);
    DGU16(0x63ea) = DGU16((uint16_t)(si + 2));

    if (DG8(0x63f4) == 0x0f && DG8(0x38af) != 0) {
        for (y = 0; DG16((uint16_t)(si + 8)) > y; y++)
            for (x = 0; DG16((uint16_t)(si + 6)) > x; x++) {
                uint8_t v = FAR8(DGU16(0x63ec), DGU16(0x63ea));

                DGU16(0x63ea)++;
                if (v != 0 && v < least)
                    least = v;
            }
    } else {
        least = 1;
    }

    DGU16(0x63ec) = DGU16(si);
    DGU16(0x63ea) = DGU16((uint16_t)(si + 2));

    hdr_seg = DGU16(0x63f0);
    hdr_off = DGU16(0x63ee);
    DGU16(0x63ee)++;

    for (y = 0; DG16((uint16_t)(si + 8)) > y; y++) {
        uint16_t at = rowbuf;

        far_memcpy(rowbuf, DGROUP_SEG, DGU16(0x63ea), DGU16(0x63ec),
                   (uint16_t)DG16((uint16_t)(si + 6)));
        DGU16(0x63ea) = (uint16_t)(DGU16(0x63ea) + DG16((uint16_t)(si + 6)));

        for (x = 0; DG16((uint16_t)(si + 6)) > x; x++) {
            uint8_t v = DG8(at);

            at++;

            if (v == 0) {
                if (di != 0) {
                    compress_row(DGU16(0x63f2), (int16_t)di);
                    di = 0;
                }
                blanks++;
                continue;
            }

            v = (uint8_t)((v - least) & DG8(0x63f4));
            DG8((uint16_t)(DGU16(0x63f2) + di)) = v;
            di++;

            if (blanks != 0) {
                emit_packed_value(blanks);
                blanks = 0;
            } else if (DGU16(0x63e2) != 0) {
                while (DGU16(0x63e2)-- != 0) {
                    FAR8(DGU16(0x63f0), DGU16(0x63ee)) = 0;
                    DGU16(0x63ee)++;
                }
                DGU16(0x63e2) = 0;
            }
        }

        if (di != 0) {
            compress_row(DGU16(0x63f2), (int16_t)di);
            di = 0;
        }

        blanks = (int16_t)(blanks - DG16((uint16_t)(si + 6)));
        DGU16(0x63e2)++;
    }

    if (di != 0)
        compress_row(DGU16(0x63f2), (int16_t)di);

    emit_packed_value(0);

    FAR8(hdr_seg, hdr_off) = least;

    dg_leave(0x14e);
}


/*
 * 0x20840
 *
 * Work out a **16.16 fixed-point step**: the span at +4..+6 divided by a
 * count, left at +4 with its low word also copied to +0.
 *
 * The span is not what it looks like. `[si]` and `[si+4]` are both zeroed
 * first, and the 32-bit subtract that follows is
 * `[si+6]:[si+4] -= [si+2]:[si]` - so with two of the four words just cleared
 * it comes to `([si+6] - [si+2]) << 16`, and the low half is zero by
 * construction. The caller puts the destination size in +6 and zero in +2, so
 * the dividend is `size << 16` and the quotient is destination pixels per
 * source pixel in 16.16.
 *
 * A count of zero or less clears +0, +4 and +6 and answers 0, so asking for no
 * steps gets a zero step rather than a division by zero.
 *
 * The sign is handled by hand - made positive, divided, negated back - which
 * is why the routine keeps a flag rather than trusting the divide.
 *
 * And +0 gets the low half of the step **except when the whole step is zero,
 * when it gets 0x8000**: a zero step would never advance, and 0x8000 is half a
 * unit here, so the smallest step is half a pixel rather than none.
 */
int16_t compute_step(uint16_t rec, int16_t count)
{
    int32_t span;
    int32_t step;
    int32_t was_negative = 0;

    if (count <= 0) {
        DG16((uint16_t)(rec + 6)) = 0;
        DG16((uint16_t)(rec + 4)) = 0;
        DG16(rec) = 0;
        return 0;
    }

    DG16(rec) = 0;
    DG16((uint16_t)(rec + 4)) = 0;

    span = (int32_t)(((uint32_t)(uint16_t)DG16((uint16_t)(rec + 6)) << 16))
         - (int32_t)(((uint32_t)(uint16_t)DG16((uint16_t)(rec + 2)) << 16));

    step = long_divide(span, (int32_t)count);

    if (step < 0) {
        step = -step;
        was_negative = 1;
    }

    DG16((uint16_t)(rec + 6)) = (int16_t)(step >> 16);
    DG16((uint16_t)(rec + 4)) = (int16_t)step;

    DG16(rec) = (step == 0) ? (int16_t)0x8000 : (int16_t)step;

    if (was_negative) {
        step = -step;
        DG16((uint16_t)(rec + 6)) = (int16_t)(step >> 16);
        DG16((uint16_t)(rec + 4)) = (int16_t)step;
    }

    return 1;
}

/*
 * 0x22790
 *
 * The distance between two entries of the scaling table at DGROUP 0x5956,
 * both indexed from the base at 0x628e: the one `n` further on, less the one
 * at the base.
 *
 * A **near** routine - `ret`, not `retf` - so its argument is at bp+4 and not
 * bp+6. The scaled blitter calls it seven times.
 */
int16_t scale_table_delta(int16_t n)
{
    uint16_t base = DGU16(0x628e);

    return (int16_t)(DG16((uint16_t)(0x5956 + 2 * (base + n)))
                     - DG16((uint16_t)(0x5956 + 2 * base)));
}

/*
 * Add the step at +4..+6 to the accumulator at +0..+2, as one 32-bit add
 * rather than two 16-bit ones. **The port's own** shape, not a transcription:
 * the original is two instructions and this is one expression.
 *
 * The original is `add [bp-0x2a], dx` then `adc [bp-0x28], ax`, with `dx` the
 * step's **low** half and `ax` its high - and the two are loaded in the other
 * order, `ax` first, which is what makes it easy to pair them up wrongly. The
 * first attempt here did exactly that, adding the high half to the low, and
 * the verifier caught it as a column table whose fifth entry was 8 where the
 * original had 3.
 */
static void step_accumulate(uint16_t rec)
{
    uint32_t acc = ((uint32_t)(uint16_t)DG16((uint16_t)(rec + 2)) << 16)
                 | (uint16_t)DG16(rec);
    uint32_t step = ((uint32_t)(uint16_t)DG16((uint16_t)(rec + 6)) << 16)
                  | (uint16_t)DG16((uint16_t)(rec + 4));

    acc += step;

    DG16(rec) = (int16_t)acc;
    DG16((uint16_t)(rec + 2)) = (int16_t)(acc >> 16);
}

/*
 * 0x227ac
 *
 * **Draw a compressed bitmap scaled.** Every part of the machine reaches the
 * screen through this: 1873 bytes, entered 57 times to paint the level-one
 * briefing alone.
 *
 * *The size arguments are also the mirrors.* A zero width or height draws
 * nothing. A **negative** one is a flip: the value is made positive with the
 * branchless `cwd`/`xor`/`sub`, the origin moved back by the new size, and the
 * mode xored with the matching mirror bit. A caller asks for a mirrored part
 * by passing a negative size.
 *
 * *Two column tables, built once.* `compute_step` divides the source width
 * into the destination width and the accumulator walks across it, filling
 * DGROUP 0x5956 with the destination x of each source column and 0x5e56 with
 * the source column of each destination x. Every row after that is a lookup,
 * and 0x628e indexes the first of them.
 *
 * *The clip is decided once per row, not per pixel.* The flag at 0x3893 is
 * copied on entry and **cleared** when the whole rectangle is inside the box,
 * so a bitmap that cannot be clipped pays no test.
 *
 * *The compression*, a byte at a time, the top two bits choosing - and this is
 * the same encoding `draw_compressed_bitmap` at 0x20185 decodes unscaled, which
 * is what settled it:
 *
 *   11  a literal run of `n` source pixels, each a **nibble**. Which half is
 *       taken is chosen without a branch on parity - the source column less
 *       the run's first is shifted right by one and the *carry* picks it - and
 *       the palette base from the header's first byte is added before the
 *       pixel reaches the row buffer. The stream advances by `(n + 1) / 2`.
 *   10  a solid run: one more byte is the colour, again plus the base.
 *   01  a move along the row; **a count of zero ends the whole bitmap**.
 *   00  the end of a row, followed by an optional second move of its low six
 *       bits shifted up by six - peeked at and only consumed if both top bits
 *       are clear.
 *
 * *The row buffer* is 0x172 bytes on the stack: a literal run is decoded into
 * it and handed to the driver whole, and a solid run never touches it.
 *
 * *A run is clipped by trimming it*, not by testing pixels: the overhang past
 * either edge is subtracted from the length and added to the buffer pointer,
 * and a run trimmed to nothing is skipped.
 *
 * **And the two mirrored trims are not written the same way.** Trimming a
 * mirrored *literal* run at the right edge computes its cut as
 * `x + clip_right` at 0x22ab4 - `03 06 96 38`, an `add` - where the mirrored
 * *solid* run at 0x22bf3 computes `x - clip_right`, `2b 06 96 38`, a `sub`.
 * The bytes were checked rather than the listing read twice. Only the second
 * is an overhang; the first is the sum of two coordinates and can only be a
 * mistake in the original. It is transcribed as the `add` it is - the rule
 * here is to transcribe, and a port that quietly corrected it would draw a
 * mirrored literal run differently from the game when one overhangs the right
 * edge of the clip box.
 *
 * *The vertical mirror is a step and the horizontal an origin.* Bit 0 makes
 * the row step -1 and moves y to the far edge; bit 1 leaves the decode alone
 * and changes where the finished row goes, and is what selects the driver's
 * mirrored entry - `stc` rather than `clc`.
 *
 * **The tag encoding was recorded inverted once and is corrected above.** The
 * first reading had 00 as the literal run and 11 as a skip, from following the
 * `jne` at 0x22988 the wrong way: it jumps when bit 7 is *set*, so the fall
 * through to 0x22c8f is the bit-7-clear case. Comparing with 0x20185, which
 * decodes the same format without scaling, is what caught it.
 */
void blit_scaled_a(uint16_t hdr, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h)
{
    uint16_t fp      = dg_enter(0x172);
    uint16_t scratch = fp;                        /* [bp-0x172] */
    uint16_t vstep32 = (uint16_t)(fp + 0x148);    /* [bp-0x2a], the accumulator */
    uint16_t vpage   = (uint16_t)(fp + 0x154);    /* [bp-0x1e] */
    uint16_t vrow    = (uint16_t)(fp + 0x156);    /* [bp-0x1c] */
    uint16_t vclip   = (uint16_t)(fp + 0x158);    /* [bp-0x1a] */
    uint16_t vrowok  = (uint16_t)(fp + 0x159);    /* [bp-0x19] */
    uint16_t vp      = (uint16_t)(fp + 0x15a);    /* [bp-0x18] */
    uint16_t vcut    = (uint16_t)(fp + 0x15c);    /* [bp-0x16] */
    uint16_t vx2     = (uint16_t)(fp + 0x15e);    /* [bp-0x14] */
    uint16_t vydir   = (uint16_t)(fp + 0x160);    /* [bp-0x12] */
    uint16_t vcol    = (uint16_t)(fp + 0x162);    /* [bp-0x10] */
    uint16_t vsrc    = (uint16_t)(fp + 0x168);    /* [bp-0xa], offset then seg */
    uint16_t vbase   = (uint16_t)(fp + 0x151);    /* [bp-0x21] */
    uint16_t vcolour = (uint16_t)(fp + 0x150);    /* [bp-0x22] */
    uint16_t vn      = (uint16_t)(fp + 0x16e);    /* [bp-4] */
    uint16_t vop     = (uint16_t)(fp + 0x170);    /* [bp-2] */
    uint16_t vx0     = (uint16_t)(fp + 0x142);    /* [bp-0x30] */
    uint16_t vxrow   = (uint16_t)(fp + 0x144);    /* [bp-0x2e] */
    uint16_t vcolrow = (uint16_t)(fp + 0x140);    /* [bp-0x32] */
    uint16_t vrowacc = (uint16_t)(fp + 0x146);    /* [bp-0x2c] */
    uint16_t vsrcrow = (uint16_t)(fp + 0x164);    /* [bp-0xe], the row's start */
    uint16_t vrepeat = (uint16_t)(fp + 0x15c);    /* [bp-0x16], reused */
    /*
     * [bp-6], and it has to be its own slot. The skipped-row loop at 0x22d94
     * keeps its scaled delta here - `mov [bp-6], ax` at 0x22db0 - while the
     * count of rows still to skip sits in [bp-0x16]. Writing the delta through
     * `vcut`, which *is* [bp-0x16], overwrote the counter with a pixel
     * distance: the loop then skipped as many source rows as the sprite was
     * wide and the decode walked off into the next rows' tags. Every scaled
     * part on the briefing screen came out as a smear.
     */
    uint16_t vdelta  = (uint16_t)(fp + 0x16c);    /* [bp-6] */
    int16_t  i, j;

    if (w == 0 || h == 0) {
        dg_leave(0x172);
        return;
    }

    if (w < 0) {
        w = (int16_t)-w;
        x = (int16_t)(x - w);
        mode ^= 2;
    }
    if (h < 0) {
        h = (int16_t)-h;
        y = (int16_t)(y - h);
        mode ^= 1;
    }

    /*
     * The same do-nothing vector `draw_compressed_bitmap` calls, kept for the
     * same reason: a build whose 0x3f72 is clear must not be silently
     * different from one whose is set.
     */
    DGU16(vpage) = DGU16(0x38a8);
    if (DG16(0x3f72) != 0)
        vm_nothing();

    DG8(vclip) = DG8(0x3893);
    if (DG8(vclip) != 0
        && x >= DG16(0x3894) && (int16_t)(x + w) <= DG16(0x3896)
        && y >= DG16(0x3898) && (int16_t)(y + h) <= DG16(0x389a))
        DG8(vclip) = 0;

    if (mode & 2)
        x = (int16_t)(x + w - 1);

    /*
     * The two column tables. `compute_step` puts the destination-per-source
     * step in the accumulator's high half, and walking it across the source
     * width fills 0x5956 with where each source column lands and 0x5e56 with
     * which source column each destination pixel came from.
     *
     * **The record's words are +2 and +6, not +0 and +2.** `compute_step`
     * clears +0 and +4 itself and takes the span from `+6 - +2`, so the caller
     * writes the two ends there; 0x2284c and 0x22854 are `[bp-0x28]` and
     * `[bp-0x24]` against a record at `[bp-0x2a]`. Writing +0 and +2 instead
     * left +6 holding whatever was there, and the step came out large enough
     * that the first source column already mapped past the end of the
     * destination - which filled 0x5e56 with -1 and made the row buffer
     * overrun. The row step below has the same two slots.
     */
    DG16((uint16_t)(vstep32 + 2)) = 0;
    DG16((uint16_t)(vstep32 + 6)) = w;
    compute_step(vstep32, DG16((uint16_t)(hdr + 6)));

    i = 0;
    j = 0;
    while (DG16((uint16_t)(hdr + 6)) >= i) {
        int16_t at = DG16((uint16_t)(vstep32 + 2));

        if (at > w)
            at = w;
        DG16((uint16_t)(0x5956 + 2 * i)) = at;

        step_accumulate(vstep32);

        while (j < at) {
            DG16((uint16_t)(0x5e56 + 2 * j)) = (int16_t)(i - 1);
            j++;
        }
        i++;
    }

    DG16(vrowacc) = 0;

    if (mode & 1) {
        DG16(vydir) = -1;
        y = (int16_t)(y + h - 1);
    } else {
        DG16(vydir) = 1;
    }

    if (DG8(vclip) != 0) {
        DG8(vrowok) = (y <= DG16(0x389a) && y >= DG16(0x3898)) ? 1 : 0;
        if (DG8(vrowok) != 0)
            DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
    } else {
        DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
    }

    DGU16((uint16_t)(vsrc + 2)) = DGU16(hdr);              /* the segment */
    DGU16(vsrc) = DGU16((uint16_t)(hdr + 2));              /* the offset */

    DG8(vbase) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
    DGU16(vsrc)++;

    DG16(vx0)   = x;
    DG16(vxrow) = x;
    DGU16(0x628e) = 0;
    DG16(vcolrow) = 0;
    DGU16(0x6290) = DGU16(0x5956);

    DGU16(vsrcrow)     = DGU16(vsrc);
    DGU16(vsrcrow + 2) = DGU16((uint16_t)(vsrc + 2));

    DG16((uint16_t)(vstep32 + 2)) = 0;
    DG16((uint16_t)(vstep32 + 6)) = (int16_t)(DG16((uint16_t)(hdr + 8)) - 1);
    compute_step(vstep32, (int16_t)(h - 1));

    for (;;) {
        DG16(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
        DGU16(vsrc)++;

        if ((DG16(vop) & 0x80) && (DG16(vop) & 0x40)) {
            /* 0x22997 - a run of nibbles, decoded into the row buffer. */
            DG16(vop) &= 0x3f;
            DG16(vn) = scale_table_delta(DG16(vop));

            if (DG16(vop) != 0) {
                int16_t  at    = DG16((uint16_t)(0x5956 + 2 * DGU16(0x628e)));
                int16_t  first = DG16((uint16_t)(0x5e56 + 2 * at));
                uint16_t out   = scratch;
                int16_t  k     = DG16(vn);
                int16_t  col   = at;

                while (k-- > 0) {
                    int16_t rel = (int16_t)(DG16((uint16_t)(0x5e56 + 2 * col))
                                            - first);
                    uint16_t byte_at = (uint16_t)((uint16_t)rel >> 1);
                    uint8_t  b = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)),
                                          (uint16_t)(DGU16(vsrc) + byte_at));

                    /*
                     * `shr` puts bit 0 in the carry and `jae` takes the even
                     * column, so an even column is the *high* nibble.
                     */
                    DG8(out) = (uint8_t)(((rel & 1) ? (b & 0x0f) : (b >> 4))
                                         + DG8(vbase));
                    out++;
                    col++;
                }

                DGU16(vsrc) = (uint16_t)(DGU16(vsrc)
                                         + ((DG16(vop) + 1) >> 1));
            }

            DGU16(0x628e) = (uint16_t)(DGU16(0x628e) + DG16(vop));
            if (DG16(vn) == 0)
                continue;

            DGU16(vp) = scratch;

            if (mode & 2) {
                DG16(vx2) = (int16_t)(x - DG16(vn));

                if (DG8(vclip) != 0) {
                    if (DG8(vrowok) == 0)
                        goto next_run;
                    if (!(DG16(vx2) >= DG16(0x3894) && x < DG16(0x3896))) {
                        if (DG16(vx2) < DG16(0x3894)) {
                            DG16(vcut) = (int16_t)(DG16(0x3894) - DG16(vx2));
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_run;
                        } else {
                            /* The `add` at 0x22ab4, as written. */
                            DG16(vcut) = (int16_t)(x + DG16(0x3896));
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_run;
                            DGU16(vp) = (uint16_t)(DGU16(vp) + DG16(vcut));
                            x = DG16(0x3896);
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)DG16(vn),
                            dgroup + DGU16(vp), DGU16(vpage), DGU16(vrow), 1);
            } else {
                DG16(vx2) = (int16_t)(x + DG16(vn));

                if (DG8(vclip) != 0) {
                    if (DG8(vrowok) == 0)
                        goto next_run;
                    if (!(x >= DG16(0x3894) && DG16(vx2) <= DG16(0x3896))) {
                        if (x < DG16(0x3894)) {
                            DG16(vcut) = (int16_t)(DG16(0x3894) - x);
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_run;
                            DGU16(vp) = (uint16_t)(DGU16(vp) + DG16(vcut));
                            x = DG16(0x3894);
                        } else {
                            DG16(vcut) = (int16_t)(DG16(vx2) - DG16(0x3896) - 1);
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_run;
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)DG16(vn),
                            dgroup + DGU16(vp), DGU16(vpage), DGU16(vrow), 0);
            }

next_run:
            x = DG16(vx2);
            continue;
        }

        if (DG16(vop) & 0x80) {
            /* 0x22b5b - a solid run: one colour byte, plus the base. */
            DG16(vop) &= 0x3f;
            DG16(vn) = scale_table_delta(DG16(vop));
            DGU16(0x628e) = (uint16_t)(DGU16(0x628e) + DG16(vop));

            DG8(vcolour) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
            DGU16(vsrc)++;

            if (mode & 2) {
                DG16(vx2) = (int16_t)(x - DG16(vn));

                if (DG8(vclip) != 0) {
                    if (DG8(vrowok) == 0)
                        goto next_solid;
                    if (!(DG16(vx2) >= DG16(0x3894) && x < DG16(0x3896))) {
                        if (DG16(vx2) < DG16(0x3894)) {
                            DG16(vcut) = (int16_t)(DG16(0x3894) - DG16(vx2));
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_solid;
                        } else {
                            DG16(vcut) = (int16_t)(x - DG16(0x3896));
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_solid;
                            x = DG16(0x3896);
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(DG8(vbase) + DG8(vcolour)),
                        (uint16_t)(x - DG16(vn) + 1), DG16(vn),
                        DGU16(vpage), DGU16(vrow));
            } else {
                DG16(vx2) = (int16_t)(x + DG16(vn));

                if (DG8(vclip) != 0) {
                    if (DG8(vrowok) == 0)
                        goto next_solid;
                    if (!(x >= DG16(0x3894) && DG16(vx2) <= DG16(0x3896))) {
                        if (x < DG16(0x3894)) {
                            DG16(vcut) = (int16_t)(DG16(0x3894) - x);
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_solid;
                            x = (int16_t)(x + DG16(vcut));
                        } else {
                            DG16(vcut) = (int16_t)(DG16(vx2) - DG16(0x3896) - 1);
                            DG16(vn) = (int16_t)(DG16(vn) - DG16(vcut));
                            if (DG16(vn) <= 0)
                                goto next_solid;
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(DG8(vcolour) + DG8(vbase)),
                        (uint16_t)x, DG16(vn), DGU16(vpage), DGU16(vrow));
            }

next_solid:
            x = DG16(vx2);
            continue;
        }

        if (DG16(vop) & 0x40) {
            /* 0x22c96 - a move along the row; a count of zero ends it all. */
            DG16(vop) &= 0x3f;
            if (DG16(vop) == 0)
                break;

            DG16(vn) = scale_table_delta(DG16(vop));
            DGU16(0x628e) = (uint16_t)(DGU16(0x628e) + DG16(vop));

            if (mode & 2)
                x = (int16_t)(x - DG16(vn));
            else
                x = (int16_t)(x + DG16(vn));
            continue;
        }

        /* 0x22cc9 - the end of a row. */
        DG16(vop) &= 0x3f;
        DG16(vn) = scale_table_delta((int16_t)-DG16(vop));
        if (DG16(vn) < 0)
            DG16(vn) = (int16_t)-DG16(vn);
        DGU16(0x628e) = (uint16_t)(DGU16(0x628e) - DG16(vop));

        if (mode & 2)
            x = (int16_t)(x + DG16(vn));
        else
            x = (int16_t)(x - DG16(vn));

        /*
         * Peek at the next tag without consuming it: only one with both top
         * bits clear is taken here, as a second move of its low six bits
         * shifted up by six.
         */
        DG16(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
        if ((DG16(vop) & 0xc0) == 0) {
            DG16(vcol) = (int16_t)(DG16(vop) & 0x3f);
            if (DG16(vcol) != 0) {
                DGU16(vsrc)++;
                DG16(vcol) = (int16_t)(DG16(vcol) << 6);
                DG16(vn) = scale_table_delta(DG16(vcol));
                DGU16(0x628e) = (uint16_t)(DGU16(0x628e) - DG16(vcol));
                if (mode & 2)
                    x = (int16_t)(x + DG16(vn));
                else
                    x = (int16_t)(x - DG16(vn));
            }
        }

        /* 0x22d45 - step the row accumulator and see how many rows it covers. */
        step_accumulate(vstep32);

        DG16(vx2) = DG16((uint16_t)(vstep32 + 2));

        if (DG16(vrowacc) == DG16(vx2)) {
            /*
             * The scaled row lands on the same destination row as the last
             * one, so this source row is not drawn at all: the source pointer,
             * x and the column index all go back to where the row began.
             */
            DGU16(vsrc)     = DGU16(vsrcrow);
            DGU16(vsrc + 2) = DGU16((uint16_t)(vsrcrow + 2));
            x = DG16(vxrow);
            DGU16(0x628e) = DGU16(vcolrow);
        } else {
            int16_t repeat = (int16_t)(DG16(vx2) - DG16(vrowacc));

            if (repeat < 0)
                repeat = (int16_t)-repeat;
            repeat--;

            DG16(vrepeat) = repeat;

            /*
             * 0x22d94 - a destination row covering more than one source row
             * still has to have those rows' tags stepped over, and their moves
             * applied, without drawing any of them.
             *
             * **Only the end-of-row tag counts.** Every branch of the body
             * jumps to the test at 0x22e6a, and just one of them - the tag
             * with both top bits clear, which is what ends a row - falls
             * through the `dec [bp-0x16]` at 0x22e67 on the way. So the
             * counter is a count of source *rows*, and the runs and moves
             * inside a row are consumed without touching it. Decrementing on
             * every tag skipped a row after one tag rather than after a row,
             * and the decode walked into the middle of the next row.
             */
            while (DG16(vrepeat) != 0) {
                DG16(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)), DGU16(vsrc));
                DGU16(vsrc)++;
                DG16(vn) = (int16_t)(DG16(vop) & 0x3f);
                DG16(vdelta) = scale_table_delta(DG16(vn));
                if (mode & 2)
                    DG16(vdelta) = (int16_t)-DG16(vdelta);

                if (DG16(vop) & 0x80) {
                    DGU16(0x628e) = (uint16_t)(DGU16(0x628e) + DG16(vn));
                    x = (int16_t)(x + DG16(vdelta));
                    if (DG16(vop) & 0x40)
                        DGU16(vsrc) = (uint16_t)(DGU16(vsrc)
                                                 + ((DG16(vn) + 1) >> 1));
                    else
                        DGU16(vsrc)++;
                } else if (DG16(vop) & 0x40) {
                    if (DG16(vn) == 0)
                        goto done;
                    DGU16(0x628e) = (uint16_t)(DGU16(0x628e) + DG16(vn));
                    x = (int16_t)(x + DG16(vdelta));
                } else {
                    DGU16(0x628e) = (uint16_t)(DGU16(0x628e) - DG16(vn));
                    x = (int16_t)(x - DG16(vdelta));

                    DG16(vop) = *FAR_PTR(DGU16((uint16_t)(vsrc + 2)),
                                         DGU16(vsrc));
                    if ((DG16(vop) & 0xc0) == 0) {
                        DG16(vcol) = (int16_t)(DG16(vop) & 0x3f);
                        if (DG16(vcol) != 0) {
                            DGU16(vsrc)++;
                            DG16(vcol) = (int16_t)(DG16(vcol) << 6);
                            DG16(vn) = scale_table_delta(DG16(vcol));
                            DGU16(0x628e) =
                                (uint16_t)(DGU16(0x628e) - DG16(vcol));
                            if (mode & 2)
                                x = (int16_t)(x + DG16(vn));
                            else
                                x = (int16_t)(x - DG16(vn));
                        }
                    }
                    DG16(vrepeat) = (int16_t)(DG16(vrepeat) - 1);
                }
            }
        }

        /* 0x22e73 - the row is finished; remember where the next one begins. */
        DGU16(vsrcrow)     = DGU16(vsrc);
        DGU16(vsrcrow + 2) = DGU16((uint16_t)(vsrc + 2));
        DG16(vrowacc) = DG16(vx2);
        DG16(vxrow)   = x;
        DGU16(vcolrow) = DGU16(0x628e);

        h--;
        if (h == 0)
            break;

        {
            int16_t back = DG16((uint16_t)(0x5956 + 2 * DGU16(0x628e)));

            if (mode & 2)
                back = (int16_t)-back;
            x = (int16_t)(DG16(vx0) + back);
        }

        y = (int16_t)(y + DG16(vydir));

        if (DG8(vclip) != 0) {
            DG8(vrowok) = (y <= DG16(0x389a) && y >= DG16(0x3898)) ? 1 : 0;
            if (DG8(vrowok) == 0)
                continue;
        }

        DGU16(vrow) = DGU16((uint16_t)(0x3f82 + 2 * y));
    }

done:
    dg_leave(0x172);
}

/*
 * 0x208f3
 *
 * Draw a plain planar bitmap scaled - the sibling of 0x227ac, 641 bytes
 * against its 1873, and the port reaches it as soon as the compressed one
 * works.
 *
 * **Its prologue is not its sibling's.** A negative size here `or`s the mirror
 * bit rather than xoring it, and does **not** move the origin back; the
 * compressed one does both. It then clamps the destination to 0x280 by 0x190,
 * the whole screen, which the other never does. Two routines doing the same
 * job for two formats, and their argument handling differs - so neither can be
 * written from the other.
 *
 * **One table, not two.** 0x5956 gets the source column for each destination
 * pixel, walked with the accumulator across the source width; the mirrored
 * case starts at `width - 1` and steps back. After the loop the last entry is
 * *incremented*, which gives the run one column of overrun to read.
 *
 * **A second table for the rows**, at 0x5e56, holding each destination row's
 * byte offset into the source - accumulated by the row's stride, which is the
 * source width shifted right by the adapter's byte-per-pixel shift at
 * 0x457a[0x38ad]. The mirrored case fills it backwards from 0x5e54.
 *
 * **The clip is applied to the rectangle, not per row**: each edge is pulled
 * in, and the left edge's overhang is kept as a *column offset* into the table
 * rather than moving the source pointer, which is what makes a clipped scale
 * still sample the right columns.
 *
 * On adapter 0x10 it programs graphics-controller registers 1, 5 and 8 before
 * drawing, which no other path here does.
 *
 * The row is drawn by **VM.OVL VGA:0x03db**, the vector at DGROUP 0x43da, with
 * `bp` pointed at `0x5956 + 2 * left_cut`; `restore_write_mode` (0x1e94c) puts
 * the graphics controller back afterwards. Both are transcribed now.
 */
void blit_scaled_b(uint16_t hdr, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h)
{
    uint16_t fp   = dg_enter(0x20);
    uint16_t rec  = fp;                 /* [bp-0x20], the 16.16 accumulator */
    int16_t  right, bottom, left, top, cut;
    int16_t  stride, plane_size;
    int16_t  i, j, row, want;
    uint16_t off, page, src_seg, src_off;

    /* A negative size is a mirror, and unlike 0x227ac it does not move the
     * origin back - the tables below are filled backwards instead. */
    if (w < 0) {
        w = (int16_t)-w;
        mode |= 2;
    }
    if (h < 0) {
        h = (int16_t)-h;
        mode |= 1;
    }

    right  = (w < 0x280) ? w : 0x280;
    bottom = (h < 0x190) ? h : 0x190;

    /*
     * The column table: for each destination pixel, the source column to take
     * it from. Mirrored, it starts at the last column and the step is negative.
     */
    if (mode & 2) {
        DG16((uint16_t)(rec + 2)) = (int16_t)(DG16((uint16_t)(hdr + 6)) - 1);
        DG16((uint16_t)(rec + 6)) = 0;
    } else {
        DG16((uint16_t)(rec + 2)) = 0;
        DG16((uint16_t)(rec + 6)) = (int16_t)(DG16((uint16_t)(hdr + 6)) - 1);
    }

    compute_step(rec, (int16_t)(right - 1));

    for (i = 0; i < right; i++) {
        DG16((uint16_t)(0x5956 + 2 * i)) = DG16((uint16_t)(rec + 2));
        step_accumulate(rec);
    }

    /* One column of overrun past the end, so the driver's run can read it. */
    DG16((uint16_t)(0x5956 + 2 * i)) =
        (int16_t)(DG16((uint16_t)(0x5956 + 2 * i)) + 1);

    /*
     * The row table, holding each destination row's *byte offset* into the
     * source rather than its row number - accumulated a stride at a time, so
     * the driver needs no multiply. The step always runs forwards; mirroring
     * writes the entries in from the far end instead.
     */
    DG16((uint16_t)(rec + 2)) = 0;
    DG16((uint16_t)(rec + 6)) = (int16_t)(DG16((uint16_t)(hdr + 8)) - 1);
    compute_step(rec, (int16_t)(bottom - 1));

    stride = (int16_t)(DG16((uint16_t)(hdr + 6))
                       >> DG8((uint16_t)(0x457a + (int8_t)DG8(0x38ad))));
    plane_size = (int16_t)(DG16((uint16_t)(hdr + 8)) * stride);

    off = 0;
    row = 0;
    for (j = 0; j < bottom; j++) {
        want = DG16((uint16_t)(rec + 2));
        step_accumulate(rec);

        while (want > row) {
            row++;
            off = (uint16_t)(off + stride);
        }

        if (mode & 1)
            DGU16((uint16_t)(0x5e54 + 2 * (bottom - j))) = off;
        else
            DGU16((uint16_t)(0x5e56 + 2 * j)) = off;
    }

    /* Only now does the rectangle become screen coordinates. */
    bottom = (int16_t)(bottom + y);
    right  = (int16_t)(right + x);
    top    = y;
    left   = x;
    cut    = 0;

    /*
     * The clip pulls each edge in - and the left edge's overhang is kept as a
     * *column offset* into the table rather than by moving the source, which
     * is what makes a clipped scale still sample the columns it would have.
     */
    if (DG8(0x3893) != 0) {
        if (right > DG16(0x3896))
            right = (int16_t)(right - (right - DG16(0x3896) - 1));
        if (bottom > DG16(0x389a))
            bottom = (int16_t)(bottom - (bottom - DG16(0x389a) - 1));
        if (top < DG16(0x3898))
            top = DG16(0x3898);
        if (left < DG16(0x3894)) {
            cut  = (int16_t)(DG16(0x3894) - left);
            left = DG16(0x3894);
        }
    }

    src_seg = DGU16(hdr);
    src_off = DGU16((uint16_t)(hdr + 2));

    if (bottom - top > 0 && right - left > 1) {
        /*
         * Set/reset off, write mode 0, and the index left on the bit mask -
         * which no other path here does, and which the driver row blit relies
         * on. `restore_write_mode` puts them back.
         */
        if (DG8(0x38b1) == 0x10) {
            io_out16(PORT_GC_INDEX, 0x0001);
            io_out16(PORT_GC_INDEX, 0x0005);
            io_out8(PORT_GC_INDEX, 0x08);
        }

        page = DGU16(0x38a8);
        if (DG16(0x3f72) != 0)
            vm_nothing();

        for (j = top; j < bottom; j++)
            vm_blit_scaled_row(
                (uint16_t)plane_size,
                (uint16_t)(0x5956 + 2 * cut),
                DGU16((uint16_t)(0x3f82 + 2 * j)),
                page, left, (int16_t)(right - left),
                (uint16_t)(DGU16((uint16_t)(0x5e56 + 2 * (j - y))) + src_off),
                src_seg);

        restore_write_mode();
    }

    dg_leave(0x20);
}

/*
 * 172c:39b7, image 0x20c07
 *
 * Clip the polygon against the window, in two passes: left and right into the
 * working arrays at 0x398c and 0x39b4, then top and bottom back into 0x393c and
 * 0x3964. Sutherland and Hodgman's, and the count at 0x3a2c is rewritten after
 * each pass.
 *
 * Each pass walks the edges with an outcode for the previous point and one for
 * this one, and there are four cases: both inside emits this point, both
 * outside on the *same* side emits nothing, and the two crossing cases emit the
 * intersection - and, when this point is the one inside, the point after it.
 * An edge that leaves through one side and comes back through the other emits
 * both intersections and no vertex, which is how a polygon wider than the
 * window keeps its shape.
 *
 * The intersection is `y0 + (y1 - y0) * (edge - x0) / (x1 - x0)`, computed with
 * `imul` and `idiv` so the product is 32 bits before the divide - the
 * coordinates are large enough that a 16-bit product would wrap.
 *
 * A polygon left with one point or none is not clipped a second time: the first
 * pass's answer is copied back and that is that.
 */
static void clip_polygon(void)
{
    int16_t si, di, bx;
    uint8_t cl, ch;
    int16_t n;

    di = 0;
    n = (int16_t)DGU16(0x3a2c);
    if (n <= 1)
        return;

    bx = (int16_t)((n - 1) * 2);

    cl = 0;
    if (DG16((uint16_t)(0x393c + bx)) < DG16(0x3894))
        cl |= 1;
    if (DG16((uint16_t)(0x393c + bx)) > DG16(0x3896))
        cl |= 2;

    for (si = 0; ; ) {
        ch = 0;
        if (DG16((uint16_t)(0x393c + si)) < DG16(0x3894))
            ch |= 1;
        if (DG16((uint16_t)(0x393c + si)) > DG16(0x3896))
            ch |= 2;

        if ((cl | ch) == 0) {
            DG16((uint16_t)(0x398c + di)) = DG16((uint16_t)(0x393c + si));
            DG16((uint16_t)(0x39b4 + di)) = DG16((uint16_t)(0x3964 + si));
            di += 2;
        } else if ((cl & ch) != 0) {
            /* Both outside the same edge: nothing survives. */
        } else if (cl == 0) {
            /* Leaving: the crossing only. */
            int16_t edge = (ch & 1) ? DG16(0x3894)
                         : (ch & 2) ? DG16(0x3896) : 0;

            if (ch & 3) {
                DG16((uint16_t)(0x398c + di)) = edge;
                DG16((uint16_t)(0x39b4 + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x3964 + bx))
                              - DG16((uint16_t)(0x3964 + si)))
                    * (int32_t)(int16_t)(edge - DG16((uint16_t)(0x393c + si)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x393c + bx))
                                         - DG16((uint16_t)(0x393c + si)))
                    + DG16((uint16_t)(0x3964 + si)));
                di += 2;
            }
        } else if (ch == 0) {
            /* Arriving: the crossing, and then the point itself. */
            int16_t edge = (cl & 1) ? DG16(0x3894)
                         : (cl & 2) ? DG16(0x3896) : 0;

            if (cl & 3) {
                DG16((uint16_t)(0x398c + di)) = edge;
                DG16((uint16_t)(0x39b4 + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x3964 + si))
                              - DG16((uint16_t)(0x3964 + bx)))
                    * (int32_t)(int16_t)(edge - DG16((uint16_t)(0x393c + bx)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x393c + si))
                                         - DG16((uint16_t)(0x393c + bx)))
                    + DG16((uint16_t)(0x3964 + bx)));
                di += 2;
            }

            DG16((uint16_t)(0x398c + di)) = DG16((uint16_t)(0x393c + si));
            DG16((uint16_t)(0x39b4 + di)) = DG16((uint16_t)(0x3964 + si));
            di += 2;
        } else {
            /* Out one side and in the other: both crossings, no vertex. */
            int16_t e1 = (cl & 1) ? DG16(0x3894)
                       : (cl & 2) ? DG16(0x3896) : 0;
            int16_t e2 = (ch & 1) ? DG16(0x3894)
                       : (ch & 2) ? DG16(0x3896) : 0;

            if (cl & 3) {
                DG16((uint16_t)(0x398c + di)) = e1;
                DG16((uint16_t)(0x39b4 + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x3964 + si))
                              - DG16((uint16_t)(0x3964 + bx)))
                    * (int32_t)(int16_t)(e1 - DG16((uint16_t)(0x393c + bx)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x393c + si))
                                         - DG16((uint16_t)(0x393c + bx)))
                    + DG16((uint16_t)(0x3964 + bx)));
                di += 2;
            }

            if (ch & 3) {
                DG16((uint16_t)(0x398c + di)) = e2;
                DG16((uint16_t)(0x39b4 + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x3964 + bx))
                              - DG16((uint16_t)(0x3964 + si)))
                    * (int32_t)(int16_t)(e2 - DG16((uint16_t)(0x393c + si)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x393c + bx))
                                         - DG16((uint16_t)(0x393c + si)))
                    + DG16((uint16_t)(0x3964 + si)));
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)DGU16(0x3a2c))
            break;
        si = (int16_t)(si * 2);
    }

    n = (int16_t)((uint16_t)di >> 1);
    DGU16(0x3a2c) = (uint16_t)n;

    if (n <= 1) {
        int16_t i;

        for (i = 0; i < n; i++) {
            DGU16((uint16_t)(0x393c + 2 * i)) = DGU16((uint16_t)(0x398c + 2 * i));
            DGU16((uint16_t)(0x3964 + 2 * i)) = DGU16((uint16_t)(0x39b4 + 2 * i));
        }
        return;
    }

    bx = (int16_t)((n - 1) * 2);
    di = 0;

    cl = 0;
    if (DG16((uint16_t)(0x39b4 + bx)) > DG16(0x389a))
        cl |= 4;
    if (DG16((uint16_t)(0x39b4 + bx)) < DG16(0x3898))
        cl |= 8;

    for (si = 0; ; ) {
        ch = 0;
        if (DG16((uint16_t)(0x39b4 + si)) > DG16(0x389a))
            ch |= 4;
        if (DG16((uint16_t)(0x39b4 + si)) < DG16(0x3898))
            ch |= 8;

        if ((cl | ch) == 0) {
            DG16((uint16_t)(0x393c + di)) = DG16((uint16_t)(0x398c + si));
            DG16((uint16_t)(0x3964 + di)) = DG16((uint16_t)(0x39b4 + si));
            di += 2;
        } else if ((cl & ch) != 0) {
            /* nothing */
        } else if (cl == 0) {
            int16_t edge = (ch & 4) ? DG16(0x389a)
                         : (ch & 8) ? DG16(0x3898) : 0;

            if (ch & 12) {
                DG16((uint16_t)(0x3964 + di)) = edge;
                DG16((uint16_t)(0x393c + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x398c + bx))
                              - DG16((uint16_t)(0x398c + si)))
                    * (int32_t)(int16_t)(edge - DG16((uint16_t)(0x39b4 + si)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x39b4 + bx))
                                         - DG16((uint16_t)(0x39b4 + si)))
                    + DG16((uint16_t)(0x398c + si)));
                di += 2;
            }
        } else if (ch == 0) {
            int16_t edge = (cl & 4) ? DG16(0x389a)
                         : (cl & 8) ? DG16(0x3898) : 0;

            if (cl & 12) {
                DG16((uint16_t)(0x3964 + di)) = edge;
                DG16((uint16_t)(0x393c + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x398c + si))
                              - DG16((uint16_t)(0x398c + bx)))
                    * (int32_t)(int16_t)(edge - DG16((uint16_t)(0x39b4 + bx)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x39b4 + si))
                                         - DG16((uint16_t)(0x39b4 + bx)))
                    + DG16((uint16_t)(0x398c + bx)));
                di += 2;
            }

            DG16((uint16_t)(0x393c + di)) = DG16((uint16_t)(0x398c + si));
            DG16((uint16_t)(0x3964 + di)) = DG16((uint16_t)(0x39b4 + si));
            di += 2;
        } else {
            int16_t e1 = (cl & 4) ? DG16(0x389a)
                       : (cl & 8) ? DG16(0x3898) : 0;
            int16_t e2 = (ch & 4) ? DG16(0x389a)
                       : (ch & 8) ? DG16(0x3898) : 0;

            if (cl & 12) {
                DG16((uint16_t)(0x3964 + di)) = e1;
                DG16((uint16_t)(0x393c + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x398c + si))
                              - DG16((uint16_t)(0x398c + bx)))
                    * (int32_t)(int16_t)(e1 - DG16((uint16_t)(0x39b4 + bx)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x39b4 + si))
                                         - DG16((uint16_t)(0x39b4 + bx)))
                    + DG16((uint16_t)(0x398c + bx)));
                di += 2;
            }

            if (ch & 12) {
                DG16((uint16_t)(0x3964 + di)) = e2;
                DG16((uint16_t)(0x393c + di)) = (int16_t)(
                    (int32_t)(DG16((uint16_t)(0x398c + bx))
                              - DG16((uint16_t)(0x398c + si)))
                    * (int32_t)(int16_t)(e2 - DG16((uint16_t)(0x39b4 + si)))
                    / (int32_t)(int16_t)(DG16((uint16_t)(0x39b4 + bx))
                                         - DG16((uint16_t)(0x39b4 + si)))
                    + DG16((uint16_t)(0x398c + si)));
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)DGU16(0x3a2c))
            break;
        si = (int16_t)(si * 2);
    }

    DGU16(0x3a2c) = (uint16_t)((uint16_t)di >> 1);
}

/*
 * 172c:3312, image 0x1f562
 *
 * One edge of the polygon, walked down the scanlines, writing the x it reaches
 * on each into the span buffer.
 *
 * `x` steps by `step` every row plus the carry out of a fractional accumulator
 * that `frac` is added to - a fixed-point DDA rather than Bresenham's error
 * term - and `di` walks the buffer four bytes a row, which is one pair of
 * span ends.
 *
 * **The original is a computed jump into an unrolled loop.** It works out
 * `0x3e28 - 7 * count` and jumps there, landing exactly `count` copies of the
 * seven-byte body from the end - about four hundred of them, which is most of
 * this routine's 3,674 bytes. That is a speed device with no observable
 * difference, so the port writes the loop.
 */
static void poly_walk(uint16_t seg, int16_t x, int16_t frac, int16_t step,
                      int16_t acc, int16_t count, uint16_t di)
{
    int16_t di_step = (int8_t)DG8(0x44e8);

    di = (uint16_t)((di << 2) + DGU16(0x44dc));

    while (count-- > 0) {
        uint32_t t;

        FAR16(seg, di) = x;
        di = (uint16_t)(di + 2 + di_step);

        t = (uint32_t)(uint16_t)acc + (uint32_t)(uint16_t)frac;
        acc = (int16_t)t;
        x = (int16_t)(x + step + (int16_t)(t >> 16));
    }
}

/*
 * 172c:3015, image 0x1f265 - an edge with no run at all.
 *
 * Both ends have the same x, so every row gets it: no fractional part and no
 * step. The two ends are put in top-to-bottom order first.
 */
static void poly_edge_vertical(uint16_t seg, int16_t x,
                               int16_t y1, int16_t y2)
{
    if (y2 <= y1) {
        int16_t t = y1;

        y1 = y2;
        y2 = t;
    }

    DG8(0x44e8) = 2;
    poly_walk(seg, x, 0, 0, 0, (int16_t)(y2 - y1 + 1), (uint16_t)y1);
}

/*
 * 172c:316f, image 0x1f3bf - an edge at exactly 45 degrees.
 *
 * One across for every one down, so again no fractional part: the step is 1 or
 * -1 by which way the x runs.
 */
static void poly_edge_diagonal(uint16_t seg, int16_t x1, int16_t x2,
                               int16_t y1, int16_t y2)
{
    /*
     * The ends are put top-first, so the swap is the one that happens when the
     * first end is *below* the second. Testing it the other way round leaves
     * the ends bottom-first, and the count below - `y2 - y1 + 1` - then comes
     * out zero or negative and the edge is not written at all. A polygon whose
     * side is at exactly 45 degrees loses that side, which is why the fault
     * hid: 45 is a special case of its own, and everything shallower or
     * steeper goes elsewhere.
     */
    if (y1 >= y2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    DG8(0x44e8) = 2;
    poly_walk(seg, x1, 0, (x1 < x2) ? 1 : -1, 0,
              (int16_t)(-(int16_t)(y1 - y2) + 1), (uint16_t)y1);
}

/*
 * 172c:3031, image 0x1f281 - an edge steeper than 45 degrees.
 *
 * Bresenham's, written out: the error starts at `2 * dx - dy`, a step that
 * takes the error non-negative moves x by one and adds `2 * (dx - dy)`, and
 * anything else adds `2 * dx`.
 *
 * The original unrolls the body six times and picks the direction by which way
 * the rows run - `di` four bytes forward or four back - which is the same two
 * loops the port writes as one with a signed step.
 */
static void poly_edge_steep(uint16_t seg, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e1, e2, x, count, sign;
    uint16_t di;

    if (x1 >= x2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)((y1 << 2) + DGU16(0x44dc));

    dy = (int16_t)(y1 - y2);
    sign = (dy >= 0) ? 0 : -1;
    if (dy < 0)
        dy = (int16_t)-dy;

    dx = (int16_t)(x2 - x1);
    e2 = (int16_t)(dx * 2);
    x = x1;
    err = (int16_t)(e2 - dy);
    e1 = (int16_t)((int16_t)((dx - dy) * 2) ^ e2);
    count = (int16_t)(dy + 1);

    /*
     * `sign` is the sign the absolute value above threw away: zero means the
     * rows run backwards through the buffer, which the original reaches by a
     * second copy of the whole unrolled body.
     */
    while (count-- > 0) {
        FAR16(seg, di) = x;
        di = (uint16_t)(di + (sign == 0 ? -4 : 4));

        if (err >= 0) {
            x++;
            err = (int16_t)(err + e1);
        } else {
            err = (int16_t)(err + e2);
        }
    }
}

/*
 * 172c:3196, image 0x1f3e6 - an edge shallower than 45 degrees,
 * the right chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **right** chain takes, where DGROUP 0x44dc is 2: it
 * writes the second slot of each row, `y * 4 + 2`, and counts x **down**.
 *
 * This and `poly_edge_shallow_left` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
static void poly_edge_shallow_right(uint16_t seg, int16_t x1, int16_t x2,
                              int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 <= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(((y1 << 1) + 1) << 1);   /* the row's second slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    FAR16(seg, di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x - 1);

    while (err < 0) {
        x = (int16_t)(x - 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        FAR16(seg, di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x - 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x - 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:3251, image 0x1f4a1 - an edge shallower than 45 degrees,
 * the left chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **left** chain takes, where DGROUP 0x44dc is 0: it
 * writes the first slot of each row, `y * 4`, and counts x **up**.
 *
 * This and `poly_edge_shallow_right` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
static void poly_edge_shallow_left(uint16_t seg, int16_t x1, int16_t x2,
                              int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 >= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(y1 << 2);                /* the row's first slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    FAR16(seg, di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x + 1);

    while (err < 0) {
        x = (int16_t)(x + 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        FAR16(seg, di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x + 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x + 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:2f69, image 0x1f219
 *
 * Draw the outline: one `clip_and_draw_line` per side, from two arrays of
 * points. The second half of the routine is the same again with the vertical
 * window and every y halved, which is the mode where a row is two scan lines -
 * the byte at DGROUP 0x3f78 says which.
 */
static void poly_outline(uint16_t xs, uint16_t ys, int16_t n)
{
    if (DG8(0x3f78) == 0) {
        while (n-- > 0) {
            clip_and_draw_line(DG16(xs), DG16(ys),
                               DG16((uint16_t)(xs + 2)),
                               DG16((uint16_t)(ys + 2)));
            xs = (uint16_t)(xs + 2);
            ys = (uint16_t)(ys + 2);
        }
        return;
    }

    DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) >> 1);
    DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) >> 1);

    while (n-- > 0) {
        clip_and_draw_line(DG16(xs), (int16_t)(DG16(ys) >> 1),
                           DG16((uint16_t)(xs + 2)),
                           (int16_t)(DG16((uint16_t)(ys + 2)) >> 1));
        xs = (uint16_t)(xs + 2);
        ys = (uint16_t)(ys + 2);
    }

    DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) << 1);
    DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) << 1);
}

/*
 * 172c:2b9d, image 0x1eded
 *
 * Fill a polygon, and outline it if the two colours differ.
 *
 * Six DGROUP arrays do the work: 0x393c and 0x3964 hold the points as given,
 * 0x398c and 0x39b4 the ones actually used, and 0x39dc and 0x3a04 a closed copy
 * kept for the outline pass - the fill destroys the working pair.
 *
 * With fewer than three points, or with filling off at DGROUP 0x389c, there is
 * nothing to fill and it draws the outline and stops.
 *
 * The fill itself is the classic one. The points are walked backwards, dropping
 * any that repeat the last, and the topmost and bottommost are found on the way
 * - by y, and by x when two share a y, which is what makes the choice
 * unambiguous. If those two turn out to have the same y the whole thing is one
 * horizontal line and goes straight to `clip_and_draw_line`.
 *
 * Otherwise the two edges leaving the top vertex are compared to see which way
 * round the polygon is wound - by slope, and the comparison is done with two
 * divisions rather than a cross product so it cannot overflow - and the arrays
 * are reversed if it is the wrong way. Then the outline is split into a left
 * chain and a right chain, each walked edge by edge into a buffer of span ends,
 * and the whole buffer handed to the driver's span filler in one call.
 *
 * This routine is hand-written assembly - BP is a general register throughout,
 * and most of its 3,674 bytes are an unrolled loop entered by computed jump -
 * so the port follows its registers rather than pretending it was compiled.
 */
void draw_polygon(int16_t n, uint16_t xs, uint16_t ys)
{
    uint16_t seg;
    int16_t ax, bx, cx, dx, si, di, bp;
    int16_t i;

    DGU16(0x44e2) = 0;
    DG8(0x44e9) = 0;

    if (n >= 0) {
        DGU16(0x3a2c) = (uint16_t)n;
        for (i = 0; i < n; i++) {
            DGU16((uint16_t)(0x393c + 2 * i)) = DGU16((uint16_t)(xs + 2 * i));
            DGU16((uint16_t)(0x3964 + 2 * i)) = DGU16((uint16_t)(ys + 2 * i));
        }
    }

    if (n < 2)
        goto out;

    if (n == 2) {
        poly_outline(0x393c, 0x3964, 1);
        goto out;
    }

    if (DG8(0x389c) == 0) {
        /* Filling is off: close the ring and draw it as lines. */
        n = (int16_t)DGU16(0x3a2c);
        DGU16((uint16_t)(0x393c + 2 * n)) = DGU16(0x393c);
        DGU16((uint16_t)(0x3964 + 2 * n)) = DGU16(0x3964);
        poly_outline(0x393c, 0x3964, n);
        goto out;
    }

    if (DG8(0x389e) != DG8(0x389d)) {
        n = (int16_t)DGU16(0x3a2c);
        DGU16(0x44e4) = (uint16_t)n;

        for (i = 0; i < n; i++) {
            DGU16((uint16_t)(0x39dc + 2 * i)) = DGU16((uint16_t)(0x393c + 2 * i));
            DGU16((uint16_t)(0x3a04 + 2 * i)) = DGU16((uint16_t)(0x3964 + 2 * i));
        }
        DGU16((uint16_t)(0x39dc + 2 * n)) = DGU16(0x393c);
        DGU16((uint16_t)(0x3a04 + 2 * n)) = DGU16(0x3964);
    }

    if (DG8(0x3893) != 0)
        clip_polygon();

    n = (int16_t)DGU16(0x3a2c);
    if (n < 2)
        goto out;
    if (n == 2) {
        poly_outline(0x393c, 0x3964, 1);
        goto out;
    }

    si = (int16_t)((n - 1) * 2);
    DGU16(0x44e0) = DGU16(0x3964);
    dx = 0x7fff;
    bx = (int16_t)0x8001;
    DGU16(0x44de) = DGU16(0x393c);
    bp = dx;
    cx = bx;
    di = 0;
    DGU16(0x44d0) = 0;
    DGU16(0x44d2) = 0;

    for (; si >= 0; si -= 2) {
        ax = DG16((uint16_t)(0x3964 + si));

        if (ax == DG16(0x44e0)
            && DG16((uint16_t)(0x393c + si)) == DG16(0x44de))
            continue;

        DG16(0x44e0) = ax;
        DG16((uint16_t)(0x39b4 + di)) = ax;

        /*
         * The tie-breaks go opposite ways, and which way is not a matter of
         * taste: the topmost keeps the point *further* right of two on the
         * same row and the bottommost the one further left, so the two chains
         * leave the vertices from opposite corners. Reading either the other
         * way round picks a different vertex to start from, and the fill can
         * still come out right - it did, pixel for pixel - while the arrays
         * the routine leaves behind do not match, which is how it was caught.
         */
        if (ax < dx
            || (ax == dx && DG16((uint16_t)(0x393c + si)) > cx)) {
            DGU16(0x44d0) = (uint16_t)di;
            dx = ax;
            cx = DG16((uint16_t)(0x393c + si));
        }

        if (ax > bx
            || (ax == bx && DG16((uint16_t)(0x393c + si)) <= bp)) {
            DGU16(0x44d2) = (uint16_t)di;
            bx = ax;
            bp = DG16((uint16_t)(0x393c + si));
        }

        ax = DG16((uint16_t)(0x393c + si));
        DG16(0x44de) = ax;
        DG16((uint16_t)(0x398c + di)) = ax;
        di += 2;
    }

    if (dx == bx) {
        /* Every point on one row: one line, and nothing to fill. */
        if (DG8(0x3f78) == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) >> 1);
            DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) << 1);
            DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) << 1);
        }
        goto out;
    }

    ax = (int16_t)((uint16_t)di >> 1);
    if (ax < 2)
        goto out;

    if (ax == 2) {
        if (DG8(0x3f78) == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) >> 1);
            DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            DG16(0x3898) = (int16_t)((uint16_t)DG16(0x3898) << 1);
            DG16(0x389a) = (int16_t)((uint16_t)DG16(0x389a) << 1);
        }
        goto out;
    }

    cx = di;
    DGU16(0x3a2c) = (uint16_t)ax;

    /*
     * Which way round is it wound? Compare the slopes of the two edges leaving
     * the top vertex. A zero rise is turned into one with a huge run so the
     * comparison still means something, and the two slopes are compared as
     * quotient-then-remainder rather than by cross-multiplying, because the
     * product would not fit.
     */
    si = (int16_t)DGU16(0x44d0);
    di = (int16_t)(si + 2);
    if (di >= cx)
        di = 0;

    dx = (int16_t)(DG16((uint16_t)(0x398c + di)) - DG16((uint16_t)(0x398c + si)));
    bp = (int16_t)(DG16((uint16_t)(0x39b4 + di)) - DG16((uint16_t)(0x39b4 + si)));
    if (bp == 0) {
        bp = 1;
        dx = (dx >= 0) ? 0x7fff : (int16_t)-0x7fff;
    }

    di = (int16_t)(si - 2);
    if (di < 0)
        di = (int16_t)(di + cx);

    ax = (int16_t)(DG16((uint16_t)(0x398c + di)) - DG16((uint16_t)(0x398c + si)));
    bx = (int16_t)(DG16((uint16_t)(0x39b4 + di)) - DG16((uint16_t)(0x39b4 + si)));
    if (bx == 0) {
        bx = 1;
        if (ax < 0) {
            ax = (int16_t)0x8001;
            goto ax_negative;
        }
        ax = (int16_t)-(int16_t)0x8001;
    }

    if (ax < 0)
        goto ax_negative;

    if (dx <= 0)
        goto reverse;
    goto compare;

ax_negative:
    if (dx >= 0)
        goto keep;

    dx = (int16_t)-dx;
    ax = (int16_t)-ax;
    {
        int16_t t = dx;

        dx = ax;
        ax = t;
        t = bx;
        bx = bp;
        bp = t;
    }

compare:
    /*
     * Each edge is a rise over a run and each keeps its own pair: `dx` goes
     * with `bp`, `ax` with `bx`, and the `xchg` above swaps the two pairs
     * whole rather than breaking them up. Dividing one edge's rise by the
     * other's run compares nothing, and the winding then comes out backwards
     * on the polygons where the two slopes happen to straddle - which is a
     * fill built from the wrong chains.
     */
    {
        uint16_t q2 = (uint16_t)ax / (uint16_t)bx;
        uint16_t r2 = (uint16_t)ax % (uint16_t)bx;
        uint16_t q1 = (uint16_t)dx / (uint16_t)bp;
        uint16_t r1 = (uint16_t)dx % (uint16_t)bp;

        if (q1 > q2)
            goto keep;
        if (q1 < q2)
            goto reverse;

        /*
         * Equal whole parts, so the remainders decide - each shifted up a
         * word and divided by its own run again, which is the original's way
         * of getting another sixteen bits of the quotient without a 32-bit
         * divide it has no instruction for.
         */
        {
            uint16_t f1 = (uint16_t)(((uint32_t)r1 << 16) / (uint16_t)bp);
            uint16_t f2 = (uint16_t)(((uint32_t)r2 << 16) / (uint16_t)bx);

            if (f1 < f2)
                goto reverse;
            if (f1 > f2)
                goto keep;
        }
    }

    /* Exactly equal: keep a copy for a second pass and go on. */
    DG8(0x44e9) = 1;
    DGU16(0x44e6) = (uint16_t)cx;
    for (i = 0; i < cx; i += 2) {
        DGU16((uint16_t)(0x39dc + i)) = DGU16((uint16_t)(0x398c + i));
        DGU16((uint16_t)(0x3a04 + i)) = DGU16((uint16_t)(0x39b4 + i));
    }

keep:
    for (i = 0; i < cx; i += 2) {
        DGU16((uint16_t)(0x393c + i)) = DGU16((uint16_t)(0x398c + i));
        DGU16((uint16_t)(0x3964 + i)) = DGU16((uint16_t)(0x39b4 + i));
    }
    goto chains;

reverse:
    for (i = 0; i < cx; i += 2) {
        DGU16((uint16_t)(0x393c + cx - 2 - i)) = DGU16((uint16_t)(0x398c + i));
        DGU16((uint16_t)(0x3964 + cx - 2 - i)) = DGU16((uint16_t)(0x39b4 + i));
    }
    DGU16(0x44d0) = (uint16_t)(cx - 2 - (int16_t)DGU16(0x44d0));
    DGU16(0x44d2) = (uint16_t)(cx - 2 - (int16_t)DGU16(0x44d2));

chains:
    /* The right chain: from the bottom vertex up to the top. */
    dx = DG16((uint16_t)(0x3964 + (int16_t)DGU16(0x44d2)));
    si = (int16_t)DGU16(0x44d0);
    di = 0;
    for (;;) {
        DG16((uint16_t)(0x398c + di)) = DG16((uint16_t)(0x393c + si));
        ax = DG16((uint16_t)(0x3964 + si));
        DG16((uint16_t)(0x39b4 + di)) = ax;
        di += 2;
        if (ax >= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    DGU16(0x44d4) = (uint16_t)((uint16_t)di >> 1);

    /* The left chain: from the top vertex down to the bottom. */
    dx = DG16((uint16_t)(0x3964 + (int16_t)DGU16(0x44d0)));
    si = (int16_t)DGU16(0x44d2);
    for (;;) {
        DG16((uint16_t)(0x398c + di)) = DG16((uint16_t)(0x393c + si));
        ax = DG16((uint16_t)(0x3964 + si));
        DG16((uint16_t)(0x39b4 + di)) = ax;
        di += 2;
        if (ax <= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    DGU16(0x44d6) = (uint16_t)(((uint16_t)di >> 1) - DGU16(0x44d4));

    seg = DGU16(0x4342);

    DGU16(0x44dc) = 2;
    DGU16(0x44da) = 0;
    ax = (int16_t)DGU16(0x44d4);

    for (;;) {
        ax--;
        if (ax == 0) {
            if (DGU16(0x44dc) != 0) {
                DGU16(0x44da) += 2;
                DGU16(0x44dc) = 0;
                ax = (int16_t)DGU16(0x44d6);
                continue;
            }
            break;
        }

        DGU16(0x44d8) = (uint16_t)ax;

        si = (int16_t)DGU16(0x44da);
        DGU16(0x44da) = (uint16_t)(si + 2);

        {
            int16_t x1 = DG16((uint16_t)(0x398c + si));
            int16_t x2 = DG16((uint16_t)(0x398e + si));
            int16_t y1 = DG16((uint16_t)(0x39b4 + si));
            int16_t y2 = DG16((uint16_t)(0x39b6 + si));
            int16_t adx = (int16_t)(x1 - x2);
            int16_t ady;

            if (adx < 0)
                adx = (int16_t)-adx;

            if (adx == 0) {
                poly_edge_vertical(seg, x1, y1, y2);
            } else {
                ady = (int16_t)(y1 - y2);
                if (ady < 0)
                    ady = (int16_t)-ady;

                if (ady == 0) {
                    /* One row: write whichever end the side wants. */
                    int16_t lo = (x1 < x2) ? x1 : x2;
                    int16_t hi = (x1 < x2) ? x2 : x1;
                    uint16_t at = (uint16_t)((y1 << 2) + DGU16(0x44dc));

                    FAR16(seg, at) = (DGU16(0x44dc) == 0) ? lo : hi;
                } else if (adx < ady) {
                    poly_edge_steep(seg, x1, x2, y1, y2);
                } else if (adx > ady) {
                    /* `cmp [0x44dc],0; jne 0x1f3e6; je 0x1f4a1`. */
                    if (DGU16(0x44dc) != 0)
                        poly_edge_shallow_right(seg, x1, x2, y1, y2);
                    else
                        poly_edge_shallow_left(seg, x1, x2, y1, y2);
                } else {
                    poly_edge_diagonal(seg, x1, x2, y1, y2);
                }
            }
        }

        ax = (int16_t)DGU16(0x44d8);
    }

    /* Hand the whole buffer to the driver's span filler in one call. */
    {
        int16_t top = DG16((uint16_t)(0x3964 + (int16_t)DGU16(0x44d0)));
        int16_t bottom = DG16((uint16_t)(0x3964 + (int16_t)DGU16(0x44d2)));
        uint16_t at = (uint16_t)((top << 2) + 0x0c);

        DGU16(0x44e2) = seg;

        FAR16((uint16_t)(seg - 1), at) = top;
        FAR16((uint16_t)(seg - 1), (uint16_t)(at + 2)) =
            (int16_t)(bottom - top + 1);

        vm_fill_spans((uint16_t)(seg - 1), at);
    }

    if (DG8(0x389e) != DG8(0x389d))
        poly_outline(0x39dc, 0x3a04, (int16_t)DGU16(0x44e4));

out:
    if (DG8(0x44e9) != 0) {
        /* The second pass, for a polygon whose two top edges had one slope. */
        DG8(0x44e9) = 0;
        cx = (int16_t)DGU16(0x44e6);
        for (i = 0; i < cx; i += 2) {
            DGU16((uint16_t)(0x398c + i)) = DGU16((uint16_t)(0x39dc + i));
            DGU16((uint16_t)(0x39b4 + i)) = DGU16((uint16_t)(0x3a04 + i));
        }
        goto reverse;
    }
}
