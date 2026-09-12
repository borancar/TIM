
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The engine under the game**, and the least single-subject file
 * here: four decompressors, the resource archive, palettes and fonts and
 * text, bitmap and polygon drawing, the keyboard, the mouse, the timer, DOS
 * memory, and the interface to the VGA driver in VM.OVL. It is named for
 * the layer it is rather than for one thing it does, because it does not do
 * one thing.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x1c250..0x248f0 - the largest of the game's own modules, 112 call targets.
 * Functions are in address order and each carries the image offset it was read
 * from.
 */
#include <string.h>

#include "tim.h"
#include "io.h"
#include "dgroup.h"

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

    if ((DG57BA.flags & 0x20) == 0) {
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
 * place and stays normalised. The port takes the destination as one pointer and
 * steps that: `cur += di` reaches the same byte every pass, because normalising
 * is about how a `seg:off` is *written down* and not about where it points, and
 * nothing outside this routine ever sees the slot. So the four-byte frame and
 * the `huge_add_to` call both go.
 *
 * The loop ends on a short read as well as on the count running out, and the
 * answer is 0 either way: nothing here reports how much it managed.
 */
int16_t read_into_huge(uint8_t far * dst, uint16_t count)
{
    uint8_t far * cur = dst;                  /* [bp+4], the caller's own far pointer,
                                          which the original steps in place */
    int16_t si = (int16_t)count;
    int16_t di = 1;

    while (si != 0 && di > 0) {
        uint16_t n = (uint16_t)(si > 0x32 ? 0x32 : si);

        di = (int16_t)game_fread(dg_ptr(dgroup, 0x5788), 1, n, FILEREC_PTR(DG57BA.word_57bc));
        si = (int16_t)(si - di);

        far_memcpy(cur, dg_ptr(dgroup, 0x5788), (uint16_t)di);

        cur += di;
    }
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
    uint16_t rec = DG5888.record_ptr;
    /* `sub`/`sbb` on the two halves - one 32-bit subtract, and **signed**,
       because the compare below is. */
    int32_t  rem = (int32_t)(RESOURCE_PTR(rec)->end - RESOURCE_PTR(rec)->in);
    uint32_t n;

    if (rem == 0)
        return 0;

    /* `xor ax,ax / cmp ax,[bp-2] / jl / jg / cmp di,[bp-4] / jbe` at 0x1c416:
       one 32-bit compare of `count` against `rem`, signed on the high word
       and unsigned on the low - the compiler putting a zero-extended `int`
       beside a `long`. **The smaller is taken**, which is the request being
       cut down to what is left.
    
       Written as two halves this read as `rem_hi > 0 || (rem_hi == 0 &&
       count > rem_lo)` choosing `rem`, which has the arms of the `min` the
       wrong way round for every `rem` above 0xffff: `jl` at 0x1c41b goes to
       0x1c42c, which is `mov ax,di` - the count. The two spellings agree
       while a chunk's remainder fits in a word, and nothing here has ever
       given it one that does not. */
    n = ((int32_t)count < rem) ? count : (uint32_t)rem;

    RESOURCE_PTR(rec)->in += n;

    if ((DG5888.flags & 0x20) != 0)
        return (int16_t)game_fread(dg_ptr(dgroup, dst), 1, (uint16_t)n,
                                   FILEREC_PTR(DG57BA.word_57bc));

    far_memcpy(dg_ptr(dgroup, dst),
               MK_FP((uint16_t)DG5888.in.seg,
                       (uint16_t)DG5888.in.off), (uint16_t)n);
    huge_add_to(&DG5888.in, (int32_t)n);

    return (int16_t)n;
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
    uint16_t rec = DG5888.record_ptr;

    RESOURCE_PTR(rec)->in += n;

    if (DG5888.word_5890 < n) {
        rec = DG5888.record_ptr;
        RESOURCE_PTR(rec)->byte_1a = (uint8_t)(RESOURCE_PTR(rec)->byte_1a + n);
        read_into_huge(dg_ptr(dgroup, DG5888.word_5892), n);
        return 0;
    }

    if ((DG57BA.flags & 0x40) != 0)
        read_into_huge(MK_FP(DG5888.out.seg, DG5888.out.off), n);
    else
        game_fseek(FILEREC_PTR(DG57BA.word_57bc), n, 1);

    DG5888.word_5890 = (int16_t)(DG5888.word_5890 - n);
    huge_add_to(&DG5888.out, (int32_t)n);

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

    if (DG5888.word_5890 < n) {
        rec = DG5888.record_ptr;
        far_memset(dg_ptr(dgroup,
                          (uint16_t)(DG5888.word_5892 + RESOURCE_PTR(rec)->byte_1a)),
                   value, (uint32_t)(int16_t)n);
        rec = DG5888.record_ptr;
        RESOURCE_PTR(rec)->byte_1a = (uint8_t)(RESOURCE_PTR(rec)->byte_1a + n);
        return 0;
    }

    if ((DG57BA.flags & 0x40) != 0)
        far_memset(MK_FP(DG5888.out.seg, DG5888.out.off), value,
                   (uint32_t)(int16_t)n);

    DG5888.word_5890 = (int16_t)(DG5888.word_5890 - n);
    huge_add_to(&DG5888.out, (int32_t)(int16_t)n);

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
    if (DG5888.word_5890 >= 1) {
        if ((DG57BA.flags & 0x40) != 0)
            *MK_FP(DG5888.out.seg, DG5888.out.off) = (uint8_t)value;

        huge_add_to(&DG5888.out, 1);
        DG5888.word_5890 = (int16_t)(DG5888.word_5890 - 1);
        return 1;
    }

    {
        uint16_t rec = DG5888.record_ptr;
        uint8_t n = RESOURCE_PTR(rec)->byte_1a;

        RESOURCE_PTR(rec)->byte_1a = (uint8_t)(n + 1);
        dg_ptr(dgroup, DG5888.word_5892)[n] = (uint8_t)value;
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
    struct far_ptr p;

    far_memset(MK_FP(DG5888.scratch.seg, DG5888.scratch.off), 0, 0x3aa1);

    DG5888.word_589e = 9;
    DG5888.word_58b6 = (int16_t)((1 << 9) - 1);

    for (i = 0xff; i >= 0; i--) {
        p = huge_add(DG5888.scratch, (int32_t)i * 2);
        *(uint16_t *)MK_FP(p.seg, p.off) = 0;

        p = huge_add(DG5888.scratch, (int32_t)i);
        p = huge_add(p, 0x2720);
        *MK_FP(p.seg, p.off) = (uint8_t)i;
    }

    DG5888.word_58a0 = 0x101;
    DG5888.word_58a4 = 0;
    DG5888.byte_58ae = 1;
    DG5888.byte_58a2 = 0;
    DG5888.word_58b2 = 0;
    DG5888.word_58b4 = 0;

    p = huge_add(DG5888.scratch, 0x3720);
    DG5888.word_58aa = (int16_t)p.seg;
    DG5888.word_58a8 = (int16_t)p.off;
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
    /*
     * The scratch buffer the string is built into, forwards, and then read out
     * of backwards. The original holds it as a segment with `di` walking in
     * and `si` walking out; both are one address here.
     */
    uint8_t far * scratch = MK_FP((uint16_t)(DG5888.scratch.seg + 0x372), 0);
    /*
     * The dictionary, two tables in one segment: a word per code at +0 and a
     * byte per code at +0x2720. Typed, so `prefix[si]` is the `si << 1` the
     * original writes by hand and `suffix[si]` is the `0x2720 + si`.
     */
    uint16_t *prefix = (uint16_t *)MK_FP(DG5888.scratch.seg, 0);
    uint8_t far * suffix = MK_FP(DG5888.scratch.seg, 0x2720);
    uint8_t far *in, *back;
    uint16_t dst_seg;
    /*
     * The output cursor. The original keeps it as `di` against a segment it
     * leaves alone, walking the offset with `inc di` and filing it back into
     * DGROUP 0x5894; here it is one address, and the two places the original
     * files it write the offset back against the segment it started from.
     *
     * **Normalising instead was tried and measured on 2026-09-10.** Writing
     * `FP_SEG`/`FP_OFF` of the cursor addresses the same byte - 424b:2b10 and
     * 44fc:0000 are both 0x44fc0 - and all 20,859 decompressed bytes were
     * identical, but the four bytes at DGROUP 0x5894 are compared and the
     * routine went from verified to DIFFERS. A pointer can only answer for
     * the normalised pair; this routine's segment is one the caller chose.
     * The scratch index that shared the `di` register is `in` above, which is
     * a different thing entirely.
     */
    uint8_t far * out;
    uint16_t si, cx;
    int16_t code;
    uint8_t al = 0;
    int16_t copying;

    if (DG5888.byte_58a2 != 0) {
        cx = (uint16_t)(DG5888.word_5890 + 1);
        dst_seg = DG5888.out.seg;
        out = MK_FP(dst_seg, (uint16_t)DG5888.out.off);
        back = scratch + (uint16_t)DG35D1.scratch_at;
        copying = (DG57BA.flags & 0x40) != 0;
        DG5888.byte_58a2 = 0;
        goto step_back;
    }

    if (DG5888.byte_58ae != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        DG5888.byte_58ae = 0;
        code = next_lzw_code();
        DG5888.word_58a6 = code;
        DG5888.word_58ac = code;
        emit_byte((uint16_t)code);
    }

    for (;;) {
        code = next_lzw_code();
        if (code < 0)
            return code;

        if (code == 0x100) {
            uint16_t p = DG5888.scratch.off;
            int16_t i;

            for (i = 0; i < 0x100; i++)
                *(uint16_t *)MK_FP(DG5888.scratch.seg,
                                     (uint16_t)(p + 2 * i)) = p;

            DG5888.word_58a4 = (int16_t)(p + 1);
            DG5888.word_58a0 = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        in = scratch;
        si = (uint16_t)code;
        DG5888.word_58b0 = code;

        if ((int16_t)si >= DG5888.word_58a0) {
            *in++ = (uint8_t)((uint16_t)DG5888.word_58ac);
            si = ((uint16_t)DG5888.word_58a6);
        }

        while (si >= 0x100) {
            *in++ = suffix[si];
            si = prefix[si];
        }

        al = suffix[si];
        *in++ = al;
        DG5888.word_58ac = al;

        cx = (uint16_t)(DG5888.word_5890 + 1);
        back = in - 1;
        dst_seg = DG5888.out.seg;
        out = MK_FP(dst_seg, (uint16_t)DG5888.out.off);
        copying = (DG57BA.flags & 0x40) != 0;

        for (;;) {
            al = *back++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                uint16_t rec;

                DG5888.out.off = (int16_t)(out - MK_FP(dst_seg, 0));
                DG35D1.scratch_at = (int16_t)(back - scratch);

                rec = DG5888.record_ptr;
                {
                    uint16_t n = RESOURCE_PTR(rec)->word_1a & 0xff;

                    RESOURCE_PTR(rec)->word_1a = (int16_t)(RESOURCE_PTR(rec)->word_1a + 1);
                    dg_ptr(dgroup, DG5888.word_5892)[n] = al;
                }

                DG5888.word_5890 = 0;
                DG5888.byte_58a2 = 1;
                return 1;
            }

            if (copying)
                *out = al;
            out++;

step_back:
            /* one forward from the load, two back, net one back - and the
               original's `js` on a 16-bit offset is this pointer stepping
               below the buffer it started at. */
            back -= 2;
            if (back < scratch)
                break;
        }

        /* 0x1cc22 - this code is done and the dictionary can grow. */
        cx--;
        DG5888.word_5890 = (int16_t)cx;
        DG5888.out.off = (int16_t)(out - MK_FP(dst_seg, 0));

        if (DG5888.word_58a0 < 0x1000) {
            uint16_t next = ((uint16_t)DG5888.word_58a0);

            prefix[next] = ((uint16_t)DG5888.word_58a6);
            DG5888.word_58a0 = (int16_t)(next + 1);
            suffix[next] = (uint8_t)((uint16_t)DG5888.word_58ac);
        }

        DG5888.word_58a6 = DG5888.word_58b0;
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
int16_t resource_read(FILE *handle, uint16_t count)
{
    uint16_t rec;
    int16_t got;

    (void)handle;

    DG5888.word_5890 = (int16_t)count;
    resource_advance();

    if (((int16_t)DG5888.word_5890) != 0) {
        uint16_t entry = DG357A.type[DG57BA.handler].read_off;

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

        if (((int16_t)DG5888.word_5890) != 0)
            resource_advance();
    }

    got = (int16_t)(count - DG5888.word_5890);

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->pos += (uint16_t)got;

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
    uint16_t ax, dx;
    const uint8_t *in;
    uint8_t ch, bl;

    if ((int16_t)((uint16_t)DG5888.word_58a0) > DG5888.word_58b6) {
        uint16_t cx = (uint16_t)(((uint16_t)DG5888.word_589e) + 1);

        DG5888.word_589e = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            DG5888.word_58b6 = 0x1000;
        else
            DG5888.word_58b6 = (int16_t)((1 << (cx & 0xff)) - 1);

        if (DG5888.word_58a4 != 0) {
            DG5888.word_589e = 9;
            DG5888.word_58b6 = 0x1ff;
            DG5888.word_58a4 = 0;
        }
    } else if (DG5888.word_58a4 != 0) {
        DG5888.word_589e = 9;
        DG5888.word_58b6 = 0x1ff;
        DG5888.word_58a4 = 0;
    } else if (DG5888.word_58b2 < DG5888.word_58b4) {
        goto extract;
    }

    {
        uint16_t width = ((uint16_t)DG5888.word_589e);
        int16_t n = read_input_block(dg_off(dgroup, DG35BC.window), width);

        if (n <= 0) {
            DG5888.word_58b4 = n;
            return -1;
        }

        DG5888.word_58b2 = 0;
        DG5888.word_58b4 = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = ((uint16_t)DG5888.word_58b2);
    bl = (uint8_t)((uint16_t)DG5888.word_589e);
    ch = (uint8_t)bitpos;

    DG5888.word_58b2 = (int16_t)(bitpos + ((uint16_t)DG5888.word_589e));

    in = &DG35BC.window[bitpos >> 3];
    ch &= 7;

    ax = *in;
    in++;
    ax = (uint16_t)(ax >> ch);
    dx = ax;

    ch = (uint8_t)(-(int8_t)(ch - 8));
    bl = (uint8_t)(bl - ch);

    if ((int8_t)bl >= 8) {
        ax = *in;
        in++;
        ax = (uint16_t)(ax << ch);
        dx |= ax;
        ch = (uint8_t)(ch + 8);
        bl = (uint8_t)(bl - 8);
    }

    ax = DG35C8.mask[bl];
    ax &= *in;
    ax = (uint16_t)(ax << ch);

    return (int16_t)(ax | dx);
}

/*
 * 0x1c649
 *
 * Select a resource by handle and unpack its entry into the globals the rest of
 * the loader reads. A **** call, so its argument is at [bp+4].
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

    entry = RESOURCE_SLOTS[handle];
    DG5888.record_ptr = (int16_t)entry;
    if (entry == 0)
        return 0;

    DG5888.scratch.seg = RESOURCE_PTR(entry)->scratch.seg;
    DG5888.scratch.off = RESOURCE_PTR(entry)->scratch.off;
    DG5888.word_5892 = (int16_t)RESOURCE_PTR(entry)->work_ptr;

    DG5888.flags = RESOURCE_PTR(entry)->kind;
    DG57BA.handler = (uint8_t)(DG5888.flags & 0x1f);

    if ((DG5888.flags & 0x20) != 0) {
        DG57BA.word_57bc = (int16_t)RESOURCE_PTR(entry)->word_06;
        DG57BA.flags = 0x20;
        return 1;
    }

    DG57BA.flags = 0;
    {
        uint32_t linear = ((uint32_t)RESOURCE_PTR(entry)->word_08 << 4)
                          + RESOURCE_PTR(entry)->word_06
                          + RESOURCE_PTR(entry)->in;
        struct far_ptr p = normalise_far_ptr_far(
            (struct far_ptr){ (uint16_t)(linear & 0xf),
                              (uint16_t)(linear >> 4) });

        DG5888.in = p;
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
    uint16_t rec = DG5888.record_ptr;

    if (RESOURCE_PTR(rec)->in == RESOURCE_PTR(rec)->end)
        return -1;

    RESOURCE_PTR(rec)->in++;

    if ((DG5888.flags & 0x20) != 0)
        return game_fgetc(FILEREC_PTR(DG57BA.word_57bc));

    {
        /* 0x5898 is `DG5888.in`, which is already a pair - the read
           cursor the decompressors walk. */
        uint32_t p = huge_post_add(&DG5888.in, 1);

        return (int16_t)(*MK_FP((uint16_t)(p >> 16), (uint16_t)p) & 0xff);
    }
}

/*
 * 0x1c6e3
 *
 * Does this NUL-terminated string contain the letter `r`?
 *
 * A **** function - it ends in `ret`, not `retf` - so its argument sits at
 * [bp+4] and the string is a DGROUP offset. The loop tests for the terminator
 * before each character and steps the pointer before testing it, so an empty
 * string answers no without reading anything.
 */
int16_t string_contains_r(const char *str)
{
    const char *s = str;

    while (*s != 0) {
        const char *at = s;
        s++;
        if (*at == 'r')
            return 1;
    }
    return 0;
}
/*
 * 0x1c705
 *
 * Free a pointer unless it is null - the whole routine.
 *
 * A **** call taking a near pointer, so its argument sits at [bp+4] rather
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

    rec = RESOURCE_SLOTS[slot];
    DG5888.record_ptr = (int16_t)rec;

    if (rec != 0) {
        free_if_set(RESOURCE_PTR(rec)->work_ptr);

        rec = DG5888.record_ptr;
        if (!huge_equal(RESOURCE_PTR(rec)->scratch.off, RESOURCE_PTR(rec)->scratch.seg, 0, 0)
            && DG3576.scratch.off == 0 && DG3576.scratch.seg == 0)
            dos_free_far(RESOURCE_PTR(rec)->scratch);
    }

    free_if_set(DG5888.record_ptr);
    RESOURCE_SLOTS[slot] = 0;

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
        if (RESOURCE_SLOTS[si] == 0)
            break;
    }

    if (si == 0x64)
        return -1;

    rec = heap_calloc_far(1, 0x21);
    DG5888.record_ptr = (int16_t)rec;
    if (rec == 0)
        return -1;

    RESOURCE_SLOTS[si] = (int16_t)rec;
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
    uint16_t near_size = 0x80;
    uint16_t far_size;
    uint16_t rec;

    if (type > 3)
        return -1;

    if (string_contains_r((const char *)dg_ptr(dgroup, name)) != 0) {
        near_size = DG357A.type[type].near_size;
        far_size = DG357A.type[type].far_size_read;
    } else {
        far_size = DG357A.type[type].far_size;
    }

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->work_ptr = (int16_t)heap_calloc_far(1, near_size);
    if (RESOURCE_PTR(rec)->work_ptr == 0)
        return -1;

    if (far_size != 0) {
        if (!huge_equal(DG3576.scratch.off, DG3576.scratch.seg, 0, 0)) {
            rec = DG5888.record_ptr;
            RESOURCE_PTR(rec)->scratch.seg = (int16_t)DG3576.scratch.seg;
            RESOURCE_PTR(rec)->scratch.off = (int16_t)DG3576.scratch.off;
            DG5888.scratch.seg = (int16_t)DG3576.scratch.seg;
            DG5888.scratch.off = (int16_t)DG3576.scratch.off;
        } else {
            struct far_ptr p = dos_alloc_bytes(far_size, 0, 0).ptr;

            rec = DG5888.record_ptr;
            RESOURCE_PTR(rec)->scratch = p;
            DG5888.scratch = p;
        }

        rec = DG5888.record_ptr;
        if (RESOURCE_PTR(rec)->scratch.off == 0 && RESOURCE_PTR(rec)->scratch.seg == 0)
            return -1;
    }

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)type;
    return 0;
}

/*
 * 0x1c8a7
 *
 * Hand over the next run of bytes from the selected resource, up to whatever
 * the caller still wants. A **** call taking nothing: everything is in the
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
    uint16_t entry = DG5888.record_ptr;
    uint16_t di = RESOURCE_PTR(entry)->byte_1b;
    uint16_t si = (uint16_t)(RESOURCE_PTR(entry)->byte_1a - di);

    if (si > DG5888.word_5890) {
        si = DG5888.word_5890;
        RESOURCE_PTR(entry)->byte_1b = (uint8_t)(RESOURCE_PTR(entry)->byte_1b + (uint8_t)si);
    } else {
        RESOURCE_PTR(entry)->byte_1a = 0;
        RESOURCE_PTR(entry)->byte_1b = 0;
    }

    if (si == 0)
        return;

    if ((DG57BA.flags & 0x40) != 0)
        far_memcpy(MK_FP((uint16_t)DG5888.out.seg,
                           (uint16_t)DG5888.out.off),
                   MK_FP((uint16_t)(dgroup_base >> 4),
                           (uint16_t)(DG5888.word_5892 + di)), si);

    DG5888.word_5890 = (int16_t)(DG5888.word_5890 - si);

    {
        uint32_t linear = ((uint32_t)DG5888.out.seg << 4) + DG5888.out.off + si;

        DG5888.out.seg = (int16_t)(linear >> 4);
        DG5888.out.off = (int16_t)(linear & 0xf);
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
int16_t open_resource(uint16_t unused, FILE *file, uint16_t name,
                      uint32_t size)
{
    int16_t slot;
    uint16_t rec;
    int16_t type;
    int32_t pos;

    (void)unused;

    slot = open_resource_slot();
    if (slot == -1)
        return -1;

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->word_06 = (int16_t)dg_off(dgroup, file);

    pos = game_ftell(file);
    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->start = (uint32_t)pos;

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->in = 5;

    if (string_contains_r((const char *)dg_ptr(dgroup, name)) == 0) {
        not_transcribed("0x1d633, opening a resource for writing");
        return -1;
    }

    type = (int16_t)(game_fgetc(file) & 0xff);
    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)type;

    if (prepare_resource_slot(type, name) == -1) {
        game_fseek(file, -1, 1);          /* 0xffff:0xffff is -1 */
        close_resource_slot((uint16_t)slot);
        return -1;
    }

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->end = size;

    game_fread(dg_ptr(dgroup, (uint16_t)(DG5888.record_ptr + 0x12)),
               1, 4, file);

    {
        uint16_t entry = DG357A.type[type].reset_off;

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

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)(RESOURCE_PTR(rec)->kind | 0x40);

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)(RESOURCE_PTR(rec)->kind | 0x20);
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

    DG5888.word_589c = 0;

    if ((DG5888.flags & 0x40) == 0) {
        not_transcribed("0x1d7c1, flushing a resource opened for writing");
        return -1;
    }

    close_resource_slot((uint16_t)handle);
    return DG5888.word_589c;
}

/*
 * 0x1d868
 *
 * Read a resource into memory. Answers what `resource_read` answered, or -1 if
 * the handle names nothing.
 *
 * Three things happen before the read. The resource is selected, which is what
 * makes DGROUP 0x588a and the rest point at it; the destination is
 * **normalised** into the pair at 0x5894, because the decompressors step it
 * with huge-pointer arithmetic that assumes it is; and bit 0x40 is set at 0x57ba,
 * which is what tells the emitters to write rather than skip.
 */
int16_t read_resource(int16_t handle, uint8_t far * dst, uint16_t count)
{
    if (select_resource(handle) == 0)
        return -1;

    /*
     * OURS, and a refusal rather than a fallback. The pair below is not a way
     * of writing the destination down, it is the **decompression cursor**:
     * fourteen sites walk it, `huge_add_to` steps it, and `decompress_lzw` and
     * `decompress_lzss` renormalise it in place with `word_5894 = di`. So the
     * destination has to be somewhere the guest can address.
     *
     * A pointer signature accepts a C local where the `seg:off` pair refused
     * one, and that is how this was got wrong before: handed a one-byte frame
     * local, `check_sound` answered a single run of blocks against fifty-five
     * and printed an empty sample list, with nothing saying why. It aborts
     * now, and `dg_is_guest` is the exact question - an address outside the
     * guest's megabyte has no pair, and every address inside it has one.
     */
    if (!dg_is_guest(dst))
        port_abort("read_resource: a destination outside guest memory has no "
                   "seg:off for the decompression cursor at DGROUP 0x5894");

    /*
     * `normalise_far_ptr_far` answers `seg + (off >> 4)` and `off & 0xf`,
     * which is the linear address split at the paragraph - and that is exactly
     * what Borland's `FP_SEG`/`FP_OFF` answer for a pointer, so the round trip
     * is not needed.
     */
    DG5888.out.seg = (int16_t)FP_SEG(dst);
    DG5888.out.off = (int16_t)FP_OFF(dst);

    DG57BA.flags = (uint8_t)(DG57BA.flags | 0x40);

    return resource_read(FILEREC_PTR((uint16_t)handle), count);
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

    rec = DG5888.record_ptr;
    return RESOURCE_PTR(rec)->size;
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
uint32_t resource_seek(int16_t handle, uint32_t by, int16_t whence)
{
    uint16_t rec;
    /* The target. Every comparison against it below is **signed** on the high
       word and unsigned on the low, which is one signed 32-bit compare - the
       original's `cmp hi / jg / jl / cmp lo / ja`. */
    uint32_t t = 0;

    if (select_resource(handle) == 0)
        return 0xffffffffu;

    rec = DG5888.record_ptr;

    if (whence == 1)
        t = RESOURCE_PTR(rec)->pos;
    else if (whence == 2)
        t = RESOURCE_PTR(rec)->size;

    t += by;

    rec = DG5888.record_ptr;
    if (RESOURCE_PTR(rec)->pos == t)
        return t;

    if ((int32_t)RESOURCE_PTR(rec)->pos > (int32_t)t) {
        /*
         * Backwards. The stream is started over - its answer is not looked at
         * - and the position is then 0, so the target *is* the distance left
         * to skip and needs no subtracting. A target at the start or before it
         * is already reached.
         */
        restart_resource_stream(handle);

        if ((int32_t)t <= 0)
            return 0;
    } else if ((int32_t)RESOURCE_PTR(rec)->size > (int32_t)t) {
        t -= RESOURCE_PTR(rec)->pos;
    } else {
        t = RESOURCE_PTR(rec)->size - RESOURCE_PTR(rec)->pos;
    }

    for (;;) {
        uint16_t n;
        int16_t got;

        if ((int32_t)t >= 0x7d00)
            n = 0x7d00;
        else
            n = (uint16_t)t;

        got = resource_read(FILEREC_PTR((uint16_t)handle), n);

        t -= (uint16_t)got;

        if (t == 0)
            break;

        rec = DG5888.record_ptr;
        {
            /* `word_06`/`word_08` is a file handle *or* the low half of a
               far pointer, so it is not a `far_ptr` field; on this path it is
               the pointer. */
            struct far_ptr p = huge_add(
                (struct far_ptr){ RESOURCE_PTR(rec)->word_06,
                                  RESOURCE_PTR(rec)->word_08 },
                (int32_t)RESOURCE_PTR(rec)->in);

            p = normalise_far_ptr_far(p);
            DG5888.in = p;
        }
    }

    rec = DG5888.record_ptr;
    return RESOURCE_PTR(rec)->pos;
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

    if (select_resource(handle) == 0 || (DG5888.flags & 0x40) == 0)
        return -1;

    {
        uint16_t entry = DG357A.type[DG57BA.handler].reset_off;

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

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->in = 5;

    rec = DG5888.record_ptr;
    if (RESOURCE_PTR(rec)->kind & 0x20) {
        uint32_t at = RESOURCE_PTR(rec)->start + 5;

        game_fseek(FILEREC_PTR(DG57BA.word_57bc), (int32_t)at, 0);
    } else {
        struct far_ptr p = huge_add(
            (struct far_ptr){ RESOURCE_PTR(rec)->word_06, RESOURCE_PTR(rec)->word_08 },
            5);

        p = normalise_far_ptr_far(p);
        DG5888.in = p;
    }

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->pos = 0;

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->byte_1b = 0;

    rec = DG5888.record_ptr;
    RESOURCE_PTR(rec)->byte_1a = 0;

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
    uint16_t rec = DG5888.record_ptr;

    DG590A.lzss_ready = 0;
    DG3600.bits = 0;
    DG3600.bit_count = 0;

    DG590A.cache_c.seg = ((int16_t)RESOURCE_PTR(rec)->scratch.seg);
    DG590A.cache_c.off = ((int16_t)RESOURCE_PTR(rec)->scratch.off);

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

    if (DG3600.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - DG3600.bit_count));
        DG3600.bits = (int16_t)(((uint16_t)DG3600.bits) | ax);
        DG3600.bit_count = (uint8_t)(DG3600.bit_count + 8);
    }

    si = DG3600.bits;
    DG3600.bits = (int16_t)(((uint16_t)DG3600.bits) << 1);
    DG3600.bit_count = (uint8_t)(DG3600.bit_count - 1);

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

    while (DG3600.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - DG3600.bit_count));
        DG3600.bits = (int16_t)(((uint16_t)DG3600.bits) | ax);
        DG3600.bit_count = (uint8_t)(DG3600.bit_count + 8);
    }

    si = ((uint16_t)DG3600.bits);
    DG3600.bits = (int16_t)(si << 8);
    DG3600.bit_count = (uint8_t)(DG3600.bit_count - 8);

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
    uint16_t rec = DG5888.record_ptr;
    uint16_t seg = RESOURCE_PTR(rec)->scratch.seg;
    uint16_t freq, prnt, son;
    int16_t i, j;

    DG590A.cache_a.seg = (int16_t)seg;
    DG590A.cache_a.off = (int16_t)(RESOURCE_PTR(rec)->scratch.off + 0x103b);
    DG590A.cache_b.seg = (int16_t)seg;
    DG590A.cache_b.off = (int16_t)(RESOURCE_PTR(rec)->scratch.off + 0x1523);
    DG5900.word_5902 = (int16_t)seg;
    DG5900.word_5900 = (int16_t)(RESOURCE_PTR(rec)->scratch.off + 0x1c7d);

    freq = DG590A.cache_a.off;
    prnt = DG590A.cache_b.off;
    son  = DG5900.word_5900;

    for (i = 0; i < 0x13a; i++) {
        *(uint16_t *)MK_FP(seg, (uint16_t)(freq + 2 * i)) = 1;
        *(uint16_t *)MK_FP(seg, (uint16_t)(son + 2 * i)) =
            (uint16_t)(i + 0x273);
        *(uint16_t *)MK_FP(seg, (uint16_t)(prnt + 2 * (i + 0x273))) =
            (uint16_t)i;
    }

    i = 0;
    for (j = 0x13a; j <= 0x272; j++) {
        *(uint16_t *)MK_FP(seg, (uint16_t)(freq + 2 * j)) =
            (uint16_t)(*(uint16_t *)MK_FP(seg, (uint16_t)(freq + 2 * i))
                       + *(uint16_t *)MK_FP(seg,
                                              (uint16_t)(freq + 2 * (i + 1))));
        *(uint16_t *)MK_FP(seg, (uint16_t)(son + 2 * j)) = (uint16_t)i;
        *(uint16_t *)MK_FP(seg, (uint16_t)(prnt + 2 * (i + 1))) =
            (uint16_t)j;
        *(uint16_t *)MK_FP(seg, (uint16_t)(prnt + 2 * i)) = (uint16_t)j;
        i += 2;
    }

    *(uint16_t *)MK_FP(seg, (uint16_t)(freq + 0x4e6)) = 0xffff;
    *(uint16_t *)MK_FP(seg, (uint16_t)(prnt + 0x4e4)) = 0;
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
    uint16_t seg = DG590A.cache_a.seg;
    uint16_t freq = DG590A.cache_a.off;
    uint16_t prnt = DG590A.cache_b.off;
    uint16_t son = DG5900.word_5900;
    int16_t i, j, k, n;

#define FREQ(x) (*(uint16_t *)MK_FP(seg, (uint16_t)(freq + 2 * (x))))
#define PRNT(x) (*(uint16_t *)MK_FP(seg, (uint16_t)(prnt + 2 * (x))))
#define SON(x)  (*(uint16_t *)MK_FP(seg, (uint16_t)(son  + 2 * (x))))

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
    uint16_t seg = DG590A.cache_a.seg;
    uint16_t freq = DG590A.cache_a.off;
    uint16_t prnt = DG590A.cache_b.off;
    uint16_t son = DG5900.word_5900;

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
    uint16_t high = (uint16_t)(DG3686.high[si] << 6);
    int16_t n = (int16_t)(DG3686.len[si] - 2);

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

    if (DG590A.lzss_ready == 0) {
        uint16_t rec;
        int16_t i;

        DG58E0.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            *MK_FP(DG590A.cache_c.seg,
                     (uint16_t)(DG590A.cache_c.off + i)) = 0x20;

        DG58E8.word_58e8 = 0xfc4;
        DG58E8.word_58ec = 0;
        DG58E8.word_58ea = 0;

        rec = DG5888.record_ptr;
        DG58E8.word_58f0 = (int16_t)(RESOURCE_PTR(rec)->size >> 16);
        DG58E8.word_58ee = (int16_t)RESOURCE_PTR(rec)->size;
        DG590A.lzss_ready = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (DG58E8.word_58ec >= DG58E8.word_58f0
            && (DG58E8.word_58ec != DG58E8.word_58f0
                || DG58E8.word_58ea >= ((uint16_t)DG58E8.word_58ee)))
            return 0;

        if (DG58E0.interrupted == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            uint16_t son = DG5900.word_5900;
            uint16_t seg = ((uint16_t)DG5900.word_5902);

            di = *(uint16_t *)MK_FP(seg, (uint16_t)(son + 0x4e4));
            while (di < 0x273)
                di = *(uint16_t *)MK_FP(
                    seg, (uint16_t)(son + 2 * (di + huff_get_bit())));

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                *MK_FP(DG590A.cache_c.seg,
                         (uint16_t)(DG590A.cache_c.off + DG58E8.word_58e8)) =
                    (uint8_t)di;
                DG58E8.word_58e8 = (int16_t)((DG58E8.word_58e8 + 1) & 0xfff);
                DG58E8.word_58ea = (int16_t)(DG58E8.word_58ea + 1);
                if (DG58E8.word_58ea == 0)
                    DG58E8.word_58ec = (int16_t)(((uint16_t)DG58E8.word_58ec) + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                DG58E0.position = (int16_t)((DG58E8.word_58e8 - pos - 1) & 0xfff);
                DG58E0.length = (int16_t)(di + 0xff03);
                DG58E0.progress = 0;
            }
        }

        DG58E0.interrupted = 0;

        while (DG58E0.progress < DG58E0.length) {
            uint16_t b = *MK_FP(
                DG590A.cache_c.seg,
                (uint16_t)(DG590A.cache_c.off
                           + ((((uint16_t)DG58E0.position) + ((uint16_t)DG58E0.progress)) & 0xfff)));

            si = emit_byte(b);

            *MK_FP(DG590A.cache_c.seg,
                     (uint16_t)(DG590A.cache_c.off + DG58E8.word_58e8)) = (uint8_t)b;
            DG58E8.word_58e8 = (int16_t)((DG58E8.word_58e8 + 1) & 0xfff);
            DG58E8.word_58ea = (int16_t)(DG58E8.word_58ea + 1);
            if (DG58E8.word_58ea == 0)
                DG58E8.word_58ec = (int16_t)(((uint16_t)DG58E8.word_58ec) + 1);

            DG58E0.progress = (int16_t)(((uint16_t)DG58E0.progress) + 1);

            if (si == 0) {
                DG58E0.interrupted = 1;
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
void blit_bitmap_thunk(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    vm_blit_bitmap(bmp, x, y, mode);
}

/*
 * 0x1e944
 *
 * A thunk into the video driver: `ljmp [0x43ca]`, which is VGA:0x271b. Same
 * arrangement as 0x1e940 - it takes three arguments rather than four, because
 * that is what its caller pushed.
 */
void blit_scaled_thunk(struct bitmap * bmp, int16_t x, int16_t y)
{
    vm_blit_scaled(bmp, x, y);
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
    if (DG3890.adapter != 0x10)
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
    DG4460.word_4460 = weight;
    DG4460.word_4462 = colour;

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
uint32_t load_palette(char *name)
{
    FILE *file = (FILE *)name;          /* a handle, or a name to open */
    /* `sub sp,0x34a`, and both halves of it are Borland locals. */
    _Alignas(2) uint8_t buf[0x300];             /* [bp-0x30a] */
    _Alignas(2) int16_t amg[0x20];              /* [bp-0x34a] */

    struct far_ptr blk = FAR_NULL;              /* [bp-0xa], [bp-8] */
    uint16_t opened;                            /* [bp-2] */
    int16_t di;
    int32_t size;

    DG4460.word_4464 = DG4466.pointer[(int16_t)DG3890.pixel_shift];

    di = 1;
    for (;;) {
        if (far_eq(DG3A2C.blocks[di], FAR_NULL))
            break;
        if (di >= 0xa)
            break;
        di++;
    }

    if (di < 0xa) {
        uint32_t chunk;

        if (file_record_valid(file) == 0) {
            opened = 1;
            file = open_file_record(name);
        } else {
            opened = 0;
        }

        /* Which palette chunk this adapter wants; entry 0 is the empty
           string, which `seek_named_chunk` refuses. */
        chunk = seek_named_chunk(
            file,
            (const char *)dg_ptr(dgroup,
                PALCHUNK.by_adapter[(int16_t)DG3890.pixel_shift]),
            0);

        if (chunk != 0xffffffffu) {
            size = DG4460.word_4464;                /* the `cwd` sign-extends it */
            blk = dos_alloc_bytes(size, 0, 0).ptr;

            if (!far_eq(blk, FAR_NULL)) {
                game_fread(buf, 1, (uint16_t)DG4460.word_4464, file);
                size = DG4460.word_4464;
                huge_move(MK_FP(blk.seg, blk.off), buf, (uint32_t)size);
            }
        } else if (DG3890.unknown_1f != 0) {
            chunk = seek_named_chunk(file, PALCHUNK.pal_amg, 0);

            if (chunk != 0xffffffffu
                && game_fread((uint8_t *)amg, 1, 0x40, file) != 0) {
                size = DG4460.word_4464;
                blk = dos_alloc_bytes(size, 0, 0).ptr;

                if (!far_eq(blk, FAR_NULL)) {
                    /* A write cursor and nothing else - only ever
                       dereferenced, so a pointer says it. */
                    uint8_t far *p = MK_FP(blk.seg, blk.off); /* [bp-4] */
                    int16_t si;

                    for (si = 0; si < 0x20; si++) {
                        int16_t w = amg[si];

                        *p++ = (uint8_t)(((w >> 8) & 0xf) << 2);
                        *p++ = (uint8_t)(((w >> 4) & 0xf) << 2);
                        *p++ = (uint8_t)((w & 0xf) << 2);
                    }
                    for (si = 0; si < 0x2a0; si++)
                        *p++ = 0;
                }
            }
        }

        if (opened != 0)
            close_file_record(file);
    }

    DG3A2C.blocks[di] = blk;

    return ((uint32_t)blk.seg << 16) | blk.off;
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
uint32_t set_palette_pointer(struct far_ptr h)
{
    int16_t idx = DG3890.pixel_shift;

    DG4460.word_4464 = DG4466.pointer[idx];

    if (far_eq(DG3A2C.blocks[0], FAR_NULL) && DG4460.word_4464 != 0) {
        int16_t bytes = (int16_t)(DG4460.word_4464 * 2);
        /* The high half was `bytes < 0 ? 0xFFFF : 0` - a `cwd`, sign-extending
           the count to the long the allocator takes. */
        struct far_ptr p = dos_alloc_bytes((uint32_t)bytes, 0, 0).ptr;

        DG3A2C.blocks[0] = p;
    }

    if (far_eq(h, FAR_NULL))
        return ((uint32_t)DG44C2.word_44c4 << 16) | DG44C2.word_44c2;

    DG44C2.word_44c4 = h.seg;
    DG44C2.word_44c2 = h.off;
    vm_load_palette(h);
    return ((uint32_t)h.seg << 16) | h.off;
}

/*
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

    if (DG3890.fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (DG3890.clip_enabled != 0) {
            int16_t d = (int16_t)(cx - DG3890.clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - DG3890.clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(DG3890.clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(DG3890.clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = MK_FP(span_buffer_seg, 0);
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

            vm_fill_spans(MK_FP(span_buffer_seg, 0));
        }
    }

    if (DG3890.fill_enabled != 0 && DG3890.fill_colour == DG3890.second_colour)
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
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    uint8_t scratch[320];   /* [bp-0x158] */
    uint8_t vb2;       /* [bp-0x18] */
    uint8_t vbase;     /* [bp-0x17] */
    int16_t vpage;     /* [bp-0x16] */
    int16_t vrow;      /* [bp-0x14] */
    uint8_t vclip;     /* [bp-0x12] */
    uint8_t vrowok;    /* [bp-0x11] */
    /* **[bp-0x10] is a cursor, not a slot.** The original keeps it in two
       frame bytes because a 16-bit machine has nowhere else to put it, and
       what it holds is the address of `scratch` above - a slot of this same
       frame, which is why nothing outside ever sees it. As a C pointer it
       walks the array directly, and `vm_blit_run` takes it as it stands
       rather than as `dgroup + offset`. */
    uint8_t * vp;                                 /* [bp-0x10] */
    uint8_t vcut[2];      /* [bp-0x0e] */
    int16_t vx2;       /* [bp-0x0c] */
    int16_t vstep;     /* [bp-0x0a] */
    int16_t vskip;     /* [bp-8] */
    int16_t vsrc[2];      /* [bp-6], offset then segment */
    uint8_t vn;        /* [bp-2] */
    uint8_t vop;       /* [bp-1] */

    /*
     * The vector at DGROUP 0x43b6 is the driver's do-nothing stub, so the page
     * comes back exactly as it went in. It is called at all only when 0x3f72
     * is set, and the port keeps the guard so that a build whose 0x3f72 is
     * clear is not silently different.
     */
    vpage = (int16_t)DG3890.page_dst_ptr;
    if (DG3F72.page_hook != 0)
        vm_nothing();

    vclip = DG3890.clip_enabled;
    if (vclip != 0
        && x >= DG3890.clip_left
        && (int16_t)(x + bmp->width) <= DG3890.clip_right
        && y >= DG3890.clip_top
        && (int16_t)(y + bmp->height) <= DG3890.clip_bottom)
        vclip = 0;

    if (mode & 1) {
        vstep = -1;
        y = (int16_t)(y + bmp->height - 1);
    } else {
        vstep = 1;
    }

    if (mode & 2)
        x = (int16_t)(x + bmp->width - 1);

    if (vclip != 0) {
        vrowok = (y <= DG3890.clip_bottom && y >= DG3890.clip_top) ? 1 : 0;
        if (vrowok != 0)
            vrow = (int16_t)ROW_BASE[y];
    } else {
        vrow = (int16_t)ROW_BASE[y];
    }

    vsrc[1] = (int16_t)bmp->data.seg;              /* the segment */
    vsrc[0] = (int16_t)bmp->data.off;              /* the offset */

    vbase = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
    vsrc[0]++;

    for (;;) {
        vop = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
        vsrc[0]++;

        if ((vop & 0x80) == 0) {
            /* 0x2058b - a move, or the end of a row. */
            if (vop & 0x40) {
                vop &= 0x3f;
                if (vop == 0)
                    break;
                if (mode & 2)
                    x = (int16_t)(x - (int8_t)vop);
                else
                    x = (int16_t)(x + (int8_t)vop);
                continue;
            }

            vop &= 0x3f;
            y = (int16_t)(y + vstep);

            if (vclip != 0) {
                vrowok = (y <= DG3890.clip_bottom && y >= DG3890.clip_top) ? 1 : 0;
                if (vrowok != 0)
                    vrow = (int16_t)ROW_BASE[y];
            } else {
                vrow = (int16_t)ROW_BASE[y];
            }

            if (mode & 2)
                x = (int16_t)(x + (int8_t)vop);
            else
                x = (int16_t)(x - (int8_t)vop);

            /*
             * Peek at the next tag without consuming it. Only a tag with both
             * top bits clear is taken here, as a move of its low six bits
             * shifted up by six; anything else is left for the loop to read
             * again.
             */
            vop = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
            if (((int16_t)(int8_t)vop & 0xc0) != 0)
                continue;

            vskip = (int16_t)((int8_t)vop & 0x3f);
            if (vskip == 0)
                continue;

            vsrc[0]++;
            vskip = (int16_t)(vskip << 6);

            if (mode & 2)
                x = (int16_t)(x + vskip);
            else
                x = (int16_t)(x - vskip);
            continue;
        }

        if ((vop & 0x40) != 0) {
            /* 0x20272 - a run of nibbles, unpacked into the scratch buffer. */
            vop &= 0x3f;
            vn = vop;
            vp = scratch;

            while (vop != 0) {
                vb2 = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
                vsrc[0]++;

                *vp = (uint8_t)(((int16_t)vb2 >> 4) + vbase);
                vp++;
                vop--;
                if (vop == 0)
                    break;

                *vp = (uint8_t)((vb2 & 0x0f) + vbase);
                vp++;
                vop--;
            }

            vp = scratch;

            if (mode & 2) {
                vx2 = (int16_t)(x - (int8_t)vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto advance;

                    while (!(vx2 >= DG3890.clip_left
                             && x < DG3890.clip_right)) {
                        if (vx2 < DG3890.clip_left) {
                            *(int16_t *)(vcut) = (int16_t)(DG3890.clip_left - vx2);
                            if (*(int16_t *)(vcut) > 0x3f)
                                goto advance;
                            vn = (uint8_t)(vn - (*vcut));
                            if ((int8_t)vn <= 0)
                                goto advance;
                            break;
                        }

                        *(int16_t *)(vcut) = (int16_t)(x - DG3890.clip_right);
                        if (*(int16_t *)(vcut) > 0x3f)
                            goto advance;
                        vn = (uint8_t)(vn - (*vcut));
                        if ((int8_t)vn <= 0)
                            goto advance;
                        vp = vp + *(int16_t *)(vcut);
                        x = DG3890.clip_right;
                        break;
                    }
                }

                vm_blit_run((uint16_t)x, vn, vp,
                            (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage }, 1);
                goto advance;
            }

            vx2 = (int16_t)(x + (int8_t)vn);

            if (vclip != 0) {
                if (vrowok == 0)
                    goto advance;

                while (!(x >= DG3890.clip_left && vx2 <= DG3890.clip_right)) {
                    if (x < DG3890.clip_left) {
                        *(int16_t *)(vcut) = (int16_t)(DG3890.clip_left - x);
                        if (*(int16_t *)(vcut) > 0x3f)
                            goto advance;
                        vn = (uint8_t)(vn - (*vcut));
                        if ((int8_t)vn <= 0)
                            goto advance;
                        vp = vp + *(int16_t *)(vcut);
                        x = DG3890.clip_left;
                        break;
                    }

                    *(int16_t *)(vcut) = (int16_t)(vx2 - DG3890.clip_right - 1);
                    if (*(int16_t *)(vcut) > 0x3f)
                        goto advance;
                    vn = (uint8_t)(vn - (*vcut));
                    if ((int8_t)vn <= 0)
                        goto advance;
                    break;
                }
            }

            vm_blit_run((uint16_t)x, vn, vp,
                        (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage }, 0);
            goto advance;
        }

        /* 0x20429 - a run of one colour. */
        vop &= 0x3f;
        vb2 = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
        vsrc[0]++;

        if (mode & 2) {
            vx2 = (int16_t)(x - (int8_t)vop);

            if (vclip != 0) {
                if (vrowok == 0)
                    goto advance;

                while (!(vx2 >= DG3890.clip_left && x < DG3890.clip_right)) {
                    if (vx2 < DG3890.clip_left) {
                        *(int16_t *)(vcut) = (int16_t)(DG3890.clip_left - vx2);
                        if (*(int16_t *)(vcut) > 0x3f)
                            goto advance;
                        vop = (uint8_t)(vop - (*vcut));
                        if ((int8_t)vop <= 0)
                            goto advance;
                        break;
                    }

                    *(int16_t *)(vcut) = (int16_t)(x - DG3890.clip_right);
                    if (*(int16_t *)(vcut) > 0x3f)
                        goto advance;
                    vop = (uint8_t)(vop - (*vcut));
                    if ((int8_t)vop <= 0)
                        goto advance;
                    x = DG3890.clip_right;
                    break;
                }
            }

            vm_span((uint16_t)(uint8_t)(vbase + vb2),
                    (uint16_t)(x - vop + 1), vop,
                    (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage });
            goto advance;
        }

        vx2 = (int16_t)(x + (int8_t)vop);

        if (vclip != 0) {
            if (vrowok == 0)
                goto advance;

            while (!(x >= DG3890.clip_left && vx2 <= DG3890.clip_right)) {
                if (x < DG3890.clip_left) {
                    *(int16_t *)(vcut) = (int16_t)(DG3890.clip_left - x);
                    if (*(int16_t *)(vcut) > 0x3f)
                        goto advance;
                    vop = (uint8_t)(vop - (*vcut));
                    if ((int8_t)vop <= 0)
                        goto advance;
                    x = (int16_t)(x + *(int16_t *)(vcut));
                    break;
                }

                *(int16_t *)(vcut) = (int16_t)(vx2 - DG3890.clip_right - 1);
                if (*(int16_t *)(vcut) > 0x3f)
                    goto advance;
                vop = (uint8_t)(vop - (*vcut));
                if ((int8_t)vop <= 0)
                    goto advance;
                break;
            }
        }

        vm_span((uint16_t)(uint8_t)(vb2 + vbase),
                (uint16_t)x, vop,
                (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage });

    advance:
        x = vx2;
    }
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
uint16_t timer_add_callback(struct far_ptr cb, uint16_t period)
{
    uint16_t mask, bx, cx;

    if (DG44EE.installed == 0)
        return 0;

    mask = DG44EE.slot_mask;
    if ((uint16_t)(mask + 1) == 0)
        return 0;

    bx = 0;
    cx = 1;
    while ((mask & 1) != 0) {
        mask >>= 1;
        cx = (uint16_t)(cx << 1);
        bx = (uint16_t)(bx + 4);
    }

    /* `bx` is the original's 4 * slot, which is how it addressed the two
       tables; the slot is `bx >> 2`, which is also what it answers. */
    DG44EE.tick[bx >> 2].period = (int16_t)period;
    DG44EE.tick[bx >> 2].left = (int16_t)period;
    DG44EE.callback[bx >> 2].off = cb.off;
    DG44EE.callback[bx >> 2].seg = cb.seg;

    /* `cli` / `sti`, around this one instruction and nothing else. */
    io_lock();
    DG44EE.slot_mask = (int16_t)(DG44EE.slot_mask | cx);
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

    DG44EE.slot_mask = (int16_t)(DG44EE.slot_mask & v);

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
    uint16_t mask = DG44EE.slot_mask;
    int32_t slot;
    int16_t n;

    n = (int16_t)(DG44EE.frame_budget - 1);
    if (n < 0)
        n = 0;
    DG44EE.frame_budget = n;

    for (slot = 0; slot < 16; slot++) {
        uint16_t used = (uint16_t)(mask & 1);

        mask = (uint16_t)(mask >> 1);

        if (used == 0) {
            if (mask == 0)
                break;
            continue;
        }

        {
            int16_t left = (int16_t)(DG44EE.tick[slot].left - 1);

            if (left == 0) {
                call_timer_handler(DG44EE.callback[slot]);
                left = DG44EE.tick[slot].period;
            }
            DG44EE.tick[slot].left = left;
        }
    }

    if (--DG44EE.divider != 0) {
        io_out8(0x20, 0x20);            /* end of interrupt */
        return;
    }

    DG44EE.divider = DG44EE.divider_reload;

    /*
     * And chain to the vector `timer_install` displaced, at ((int16_t)S1CS.old_int8.off). That
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
void blit_rows_thunk(struct far_ptr src, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_blit_rows(src, x, y, w, h);
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
    if (DG458C.word_458c == 0) {
        uint32_t v;

        v = dos_getvect(0x09);
        S1CS.word_4e3c = (int16_t)v;
        S1CS.word_4e3e = (int16_t)(v >> 16);

        v = dos_getvect(0x1c);
        S1CS.word_4e40 = (int16_t)v;
        S1CS.word_4e42 = (int16_t)(v >> 16);

        dos_setvect(0x09, 0x4f46, (uint16_t)(S1C25 >> 4));

        if (hook_timer != 0)
            dos_setvect(0x1c, 0x5136, (uint16_t)(S1C25 >> 4));

        DG471B.pcjr_keyboard = 0;

        if (detect_pcjr() != 0)
            not_transcribed("0x210f3, the PCjr keyboard path - INT 15h, the "
                            "keyboard type at 0040:0096, and the remapping "
                            "at 0x2110c");

        DG458C.word_458c = 1;
    }

    FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) & 0xdf);

    if (DG458C.byte_458d != 0)
        FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) | 0x40);

    return DG458C.word_458c;
}

/*
 * 0x21196   (segment 1c25, offset 0x4f46 - where `install_keyboard` puts it)
 *
 * **The game's own keyboard interrupt, and it does the whole job.** It does
 * not chain to the BIOS: the fall-through is `mov al,0x20 / out 0x20,al /
 * iret`, so once this is installed nothing else sees a keystroke. Three things
 * come out of it, and until 2026-09-06 the port had only the first:
 *
 *   - the **BIOS ring** at 0040:001c, which `bios_read_key` drains. That is
 *     how Tab, X, Y, `-`, `=` and the music keys reach the game, and the port
 *     used to fill it from SDL directly.
 *   - the **per-scancode array at DGROUP 0x468c**, which is where
 *     `timer_callback` reads the arrows, Space, Enter and Esc, and where
 *     `game_screen` reads Alt with V. Nothing filled it, so all of those were
 *     dead - `incredible-machine/READ.ME` documents them and that is how the
 *     gap was found.
 *   - the **BIOS shift flags** at 0040:0017.
 *
 * The scancode is read from port 0x60 and the keyboard acknowledged by pulsing
 * bit 7 of port 0x61 and putting it back.
 *
 * **Eleven keys are remapped** through the pair of tables at DGROUP 0x4705 and
 * 0x4710 - the second is the first plus 0xb - and the scan stops at the first
 * match. With `0x471b` set instead, two keys are remapped in code rather than
 * by table: 0x29 becomes Up and 0x2b becomes Left, which is a keyboard without
 * a cursor pad.
 *
 * The state byte is built by a shift rather than a test: DH starts 0xff, DL
 * takes the code, `shl dx,1` moves the release bit out of DL into DH's bottom
 * bit and leaves the scancode in DL after `shr dl,1`. So DH is 0xfe pressed
 * and 0xff released, and `and`/`xor 1` then sets bit 0 on a press and toggles
 * it on a release. Anything at or above 0x59 is dropped before that.
 *
 * **The ring holds one key.** The fullness test is `head + 2 == tail`, not the
 * usual `tail + 2 == head`, so a second key is refused while one is still
 * unread. That is not a transcription slip: it is what the bytes say, and it
 * is why the port's own `io_key_press` - which filled the whole ring - was not
 * the same thing.
 *
 * Ctrl with 0x19b, or Ctrl-Alt with 0x5380, unwinds the last ring entry and
 * calls `game_teardown` with 0 or 1. That is the only path here that does not
 * simply acknowledge and return.
 */
void keyboard_isr(void)
{
    uint16_t raw, bx, di, cx;
    uint8_t al, bl, dl, dh, cl, ch, p61;

    al  = io_in8(0x60);
    raw = al;
    p61 = io_in8(0x61);
    io_out8(0x61, (uint8_t)(p61 | 0x80));
    io_out8(0x61, p61);

    al = (uint8_t)(raw & 0x7f);
    bl = (uint8_t)(raw & 0x80);

    if (DG3890.unknown_1c == 1) {
        if (DG471B.pcjr_keyboard == 1) {
            if (al == 0x29)
                al = 0x48;
            if (al == 0x2b)
                al = 0x4b;
        } else {
            int16_t i;

            for (i = 0; i < 0xb; i++)
                if (DG458C.pcjr_from[i] == al) {
                    al = DG458C.pcjr_to[i];
                    break;
                }
        }
    }

    al = (uint8_t)(al | bl);
    dh = (uint8_t)(0xfe | (al >> 7));
    dl = (uint8_t)(al & 0x7f);

    if (dl >= 0x59) {
        io_out8(0x20, 0x20);
        return;
    }

    bx = dl;
    dl = DG458C.state[bx];          /* the state as it was */
    dh = (uint8_t)((dh & dl) ^ 1);
    DG458C.state[bx] = dh;

    if (DG471B.pcjr_keyboard == 1 && (bx == 0x3a || bx == 0x45))
        al = (uint8_t)bx;                       /* Caps and Num, never a release */

    if ((al & 0x80) != 0) {
        /* ---- a key coming up ---- */
        if ((dl & 0xf8) != 0) {
            cx = (uint16_t)(dl >> 3);
            di = (uint16_t)(cx & 1);
            cl = (uint8_t)(cx >> 1);
            ch = DG458C.held[di];
            if (ch == cl)
                DG458C.held[di] = 0;
        }

        DG458C.word_458e = 0;

        al = DG458C.ascii[al & 0x7f];
        if ((al & 0x80) != 0 && (al & 0x70) == 0) {
            al ^= 0x7f;
            FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) & al);
        }
        io_out8(0x20, 0x20);
        return;
    }

    /* ---- a key going down ---- */
    if ((dl & 0xf8) != 0) {
        cx = (uint16_t)(dl >> 3);
        di = (uint16_t)(cx & 1);
        DG458C.held[di] = (uint8_t)(cx >> 1);
    }

    cl = al;                                    /* the scancode, for AH later */
    di = al;
    al = DG458C.ascii[di];

    if ((al & 0x80) != 0) {
        al &= 0x7f;
        if ((al & 0x70) == 0) {
            FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) | al);
            io_out8(0x20, 0x20);
            return;
        }
        if ((al & 0x40) == 0 || DG458C.byte_458d == 0) {
            if ((dl & 1) == 0)
                FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) ^ al);
        }
        io_out8(0x20, 0x20);
        return;
    }

    if ((FAR8(0x40, 0x17) & 4) != 0) {
        al |= 0x80;
        if ((dl & 4) != 0)
            al = (uint8_t)(al - 0x20);
    } else if ((FAR8(0x40, 0x17) & 0x40) != 0) {
        if ((dl & 4) != 0)
            al = (uint8_t)(al - 0x20);
    } else if ((FAR8(0x40, 0x17) & 3) != 0) {
        al = DG458C.shifted[di];
    }

    {
        uint16_t ax = (uint16_t)((cl << 8) | al);
        uint16_t head, tail;
        int16_t full = 0;

        DG458C.word_458e = ax;

        head = (uint16_t)FAR16(0x40, 0x1a);
        tail = (uint16_t)FAR16(0x40, 0x1c);

        if (head == 0x3c) {
            if (tail == 0x1e)
                full = 1;
        } else if ((uint16_t)(head + 2) == tail) {
            full = 1;
        }

        if (!full) {
            FAR16(0x40, tail) = (int16_t)ax;
            if (tail == 0x3c)
                tail = 0x1c;
            tail = (uint16_t)(tail + 2);
            FAR16(0x40, 0x1c) = (int16_t)tail;
        }

        if ((ax >> 8) == 0x20 && (FAR8(0x40, 0x17) & 4) != 0) {
            io_out8(0x20, 0x20);
            return;
        }

        bx = 0;
        if (ax != 0x19b) {
            bx = 1;
            if (ax != 0x5380 || (FAR8(0x40, 0x17) & 8) == 0) {
                io_out8(0x20, 0x20);
                return;
            }
        }
        if ((FAR8(0x40, 0x17) & 4) == 0) {
            io_out8(0x20, 0x20);
            return;
        }

        tail = (uint16_t)(tail - 2);
        if (tail == 0x1c)
            tail = 0x3c;
        FAR16(0x40, tail) = 0;
        FAR16(0x40, 0x1a) = FAR16(0x40, 0x1c);

        io_out8(0x20, 0x20);
        game_teardown((int16_t)bx);
    }
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
    return (int16_t)(DG458C.state[index] & 1);
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
        struct far_ptr cur = DG618A.fonts;

        /* 0000:0000, which is the guest's first byte and not a C null. */
        if (far_eq(cur, FAR_NULL))
            return 0;

        /* Which slot holds the same pointer as slot 0. */
        for (di = 1; di < 0x14; di++)
            if (far_eq(FONTSLOT[di], cur))
                break;

        return (uint16_t)di;
    }

    if (table_618a_in_use(slot) == 0)
        return 0;

    di = slot;

    DG6176.kind[0] = DG6176.kind[slot];
    DG3890.font_table_34[0] = DG3890.font_table_34[slot];
    DG3890.font_table_48[0] = DG3890.font_table_48[slot];
    DG627A.underline_row[0] = DG627A.underline_row[slot];
    DG3890.font_table_5c[0] = DG3890.font_table_5c[slot];
    DG3890.font_table_70[0] = DG3890.font_table_70[slot];

    DG618A.fonts  = FONTSLOT[slot];
    DG61DA.widths = WIDTHSLOT[slot];
    DG622A.slot = MIDSLOT[slot];

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
union far_or_size dos_alloc_bytes(uint32_t size, uint16_t unused,
                                  uint16_t flags)
{
    (void)unused;
    uint16_t paras, remainder, seg, largest;
    int32_t failed;

    /* **One Borland `long`**, low word at [bp+6]. 0x21ad1 shifts the pair
       right four with `shr ax,1 / rcr bx,1` four times over, which is a
       32-bit shift and not two 16-bit ones; the test above it is
       `cmp ax,bx / jne / cmp ax,0xffff`, the pair against 0xffffffff. */
    if (size == 0xFFFFFFFFu) {
        /* The "how much is free" question. */
        io_dos_alloc(0xFFFF, &largest, &failed);
        {
            union far_or_size r;

            r.bytes = (uint32_t)largest << 4;
            return r;
        }
    }

    /* Bytes to paragraphs, rounded up. The high half of the shifted pair
       is dropped - DOS takes the count in BX alone, and AH is loaded with
       0x48 over what was in AX - so a request above a megabyte would
       truncate here exactly as it does in the original. */
    remainder = (uint16_t)(size & 0x0F);
    paras = (uint16_t)(size >> 4);
    if (remainder != 0)
        paras = (uint16_t)(paras + 1);

    seg = io_dos_alloc(paras, &largest, &failed);
    if (failed) {
        union far_or_size r;

        r.ptr = FAR_NULL;
        return r;
    }

    if (flags & 1)
        far_memset(MK_FP(seg, 0), 0, size);

    {
        union far_or_size r;

        r.ptr.off = 0;
        r.ptr.seg = seg;
        return r;
    }
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
void dos_free_far(struct far_ptr block)
{
    io_dos_free(block.seg);
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

    if (DG3890.clip_enabled != 0) {
        /* top */
        edge = DG3890.clip_top;
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
        edge = DG3890.clip_left;
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
        edge = DG3890.clip_bottom;
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
        edge = DG3890.clip_right;
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

    if (DG48DA.mouse_taken != 0)
        return 0;

    present = io_mouse_reset();
    DG48DA.mouse_taken = (uint8_t)(-(int16_t)present);

    if (present == 0)
        return 0;

    io_mouse_move_to(0x7fff, 0x7fff);
    io_mouse_show();
    io_mouse_hide();
    io_mouse_set_speed(8, 8);
    io_mouse_move_to(0, 0);

    mouse_set_ranges(0, 0, ((uint16_t)DG3F78.screen_width), ((uint16_t)DG3F78.screen_height));

    io_mouse_set_handler(0x1f, 0x5d7f, (uint16_t)(S1C25 >> 4));

    if (((uint8_t)DG3890.pixel_shift) == 8) {
        DG48DA.word_48e6 = DG48DA.quarter_a;
        DG48DA.word_48e8 = DG48DA.quarter_b;
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
    DG48DA.gc_0_1 = (int16_t)v;
    io_out8(PORT_GC_INDEX, 0);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 1);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    DG48DA.gc_4 = (int16_t)v;

    io_out8(PORT_GC_INDEX, 5);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, DG48DA.word_48e8);
    vga_write(0xffff, DG48DA.word_48e8);          /* park the latches */
    io_out8(PORT_GC_DATA, DG48DA.word_48e6);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    DG48DA.gc_8 = (int16_t)v;
    io_out8(PORT_GC_DATA, 0xff);

    io_out8(PORT_GC_INDEX, 3);
    DG48DA.gc_3 = io_in8(PORT_GC_DATA);
    io_out8(PORT_GC_DATA, 0);

    v = (uint16_t)(io_in8(PORT_SEQ_INDEX) << 8);
    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)(v | io_in8(PORT_SEQ_DATA));
    DG48DA.seq_map_mask = (int16_t)v;
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
    v = (uint16_t)DG48DA.seq_map_mask;
    io_out8(PORT_SEQ_DATA, (uint8_t)v);
    io_out8(PORT_SEQ_INDEX, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 3);
    io_out8(PORT_GC_DATA, DG48DA.gc_3);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)DG48DA.gc_8;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 5);
    io_out8(PORT_GC_DATA, DG48DA.word_48e8);
    (void)vga_read(0xffff);                  /* pick the latches back up */
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)DG48DA.gc_4;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 1);
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)DG48DA.gc_0_1;
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
void mouse_set_user_handler(struct far_ptr h)
{
    DG4740.word_4744 = h.off;
    DG4740.word_4746 = h.seg;
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
    DG48DA.buttons = (uint8_t)buttons;
    DG4740.word_4740 = x;
    DG4740.word_4742 = y;

    if ((DG4740.word_4744 | DG4740.word_4746) == 0)
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
void read_pair_4740(int16_t *out_a, int16_t *out_b)
{
    if (DG48DA.mouse_taken == 0)
        return;
    *out_a = (int16_t)(DG4740.word_4740 >> 2);
    *out_b = (int16_t)(DG4740.word_4742 >> 2);
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
    uint16_t v = DG48DA.mouse_taken;

    if (v == 0)
        return 0;

    v = DG48DA.buttons;
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
 * A **** routine taking and answering registers, so the port passes them
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
 * where, which is exactly `memmove`'s contract.
 *
 * So the port takes two pointers and a count. The original's own `seg:off`
 * arithmetic was the *only* thing that forced its source to be a guest
 * address - it never read the source, it indexed memory by the linear address
 * it computed - and with that gone `load_palette` hands it an ordinary C
 * array.
 *
 * One thing does not survive the change. The routine answered the destination
 * pair it was given, unnormalised, in DX:AX; a pointer cannot spell an
 * unnormalised pair, so it answers the destination pointer and `routines.def`
 * says RET_NONE. Nothing reads the answer: `load_palette` is the only caller
 * and drops it, and `syms.c` does not dispatch this routine.
 *
 * The two dispatch words *do* survive, and getting them wrong was the first
 * thing the verifier said - `0x5f11/0x5f86` against `0x5f23/0x5f6f`, two bytes
 * of a 578-byte write. They are written from the same comparison the original
 * makes, with a converted frame standing at DGROUP; see below. They are *data*
 * that happens to sit in a code segment, the same as the saved timer vector
 * further up this module, and `S1C16` is how the port reaches that. Nothing
 * reads them back, but they are compared.
 */
uint8_t far * huge_move(uint8_t far * dst, const uint8_t far * src, uint32_t count)
{
    /* The original's dispatch words, stored for the comparison's sake only. */
    /* Stored going up, and overwritten below if the copy has to go down. */
    S1CS.word_5f99 = 0x5f11;
    S1CS.word_5f9b = 0x5f86;

    /*
     * The original compares the two *linear* addresses, and that is a question
     * about where the operands are, not about their bytes. A pointer into
     * guest memory answers it directly. A converted frame has no linear
     * address of its own and stands for the frame it replaced, which was in
     * DGROUP - so DGROUP is where it is, for this question.
     */
    if ((dg_is_guest(src) ? src : dgroup)
        < (dg_is_guest(dst) ? (const uint8_t *)dst : dgroup)) {
        S1CS.word_5f99 = 0x5f23;
        S1CS.word_5f9b = 0x5f6f;
    }

    memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, count);

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
void far_memcpy(uint8_t far * dst, const uint8_t far * src, uint16_t count)
{
    uint16_t words;

    if (count == 0)
        return;

    /*
     * The original normalises both pairs and then steps the offsets 16-bit,
     * which is `rep movsw` on a machine with segments. A host pointer is that
     * normalised address already, and the wrap goes with the pair - measured
     * across the intro, the briefing, the picker and all twenty-eight level
     * snapshots, neither end ever reached `off + count > 0x10000`.
     *
     * The words-then-a-byte shape is kept: it decides which byte of an odd
     * count is copied last, and the word is read at an odd address the
     * way the guest's unaligned `movsw` does.
     */
    words = (uint16_t)(count >> 1);
    while (words--) {
        *(int16_t *)(dst) = *(int16_t *)(src);
        src += 2;
        dst += 2;
    }
    if (count & 1)
        *dst = *src;
}
/*
 * 0x22300
 *
 * Set `count` bytes to the low byte of `value`, starting at a far pointer.
 *
 * **The machinery is not reconstructed**, on the same reasoning as `huge_move`
 * above: the original works in chunks of at most 0x7d00 bytes, renormalising
 * the pointer at the top of each so an offset can never carry past 64K, writes
 * one byte first where that makes the destination even, and then moves words
 * with both halves of the value. Every part of that is there to make the fill
 * fast on a 16-bit machine, and none of it changes which bytes end up holding
 * what - which is `memset`'s contract.
 *
 * The offset stepping was the only thing that made the destination a guest
 * address: `off = (uint16_t)(off + 2)` is a 16-bit add, and the renormalisation
 * is what keeps it from wrapping. Neither survives as a pointer and neither
 * needs to.
 */
void far_memset(uint8_t far * dst, uint16_t value, uint32_t count)
{
    memset((void *)(uintptr_t)dst, (int)(value & 0xff), count);
}
/*
 * 0x22386
 *
 * The far-callable face of `normalise_far_ptr` at 0x22161: load the pointer
 * into AX and DX, call the near routine, and let its registers be the result.
 * So this answers a normalised far pointer in DX:AX, like any other far
 * routine returning a long.
 */
struct far_ptr normalise_far_ptr_far(struct far_ptr p)
{
    /* Through locals rather than `&p.off`: `struct far_ptr` is packed, and
       taking the address of a packed member is what -Waddress-of-packed-member
       is for. The two words are 2-aligned in practice, but the warning is
       right that nothing guarantees it. */
    uint16_t off = p.off, seg = p.seg;

    normalise_far_ptr(&off, &seg);

    p.off = off;
    p.seg = seg;
    return p;
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
    if (DG3890.clip_enabled != 0) {
        if (x < DG3890.clip_left)
            return -1;
        if (x > DG3890.clip_right)
            return -1;
        if (y < DG3890.clip_top)
            return -1;
        if (y > DG3890.clip_bottom)
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
uint16_t load_font(char *name)
{
    int16_t size[2];      /* [bp-4], read into by fread */

    FILE *di = (FILE *)name;          /* a handle, or a name to open */
    uint16_t opened = 0;                        /* [bp-2]  */
    int16_t handle;                             /* [bp-6]  */
    int16_t failed;                             /* [bp-8]  */
    struct far_ptr blk = FAR_NULL;              /* [bp-0xa], [bp-0xc] */
    uint16_t p;                                 /* [bp-0xe] */
    int16_t si;

    si = 2;
    for (;;) {
        if (far_eq(FONTSLOT[si], FAR_NULL))
            break;
        if (si >= 0x14)
            break;
        si++;
    }

    if (si >= 0x14) {
        return 0;
    }

    if (file_record_valid(di) == 0) {
        opened = 1;
        di = open_file_record(name);
    } else {
        opened = 0;
    }

    if (seek_named_chunk(di, (const char *)dg_ptr(dgroup,
                                                  DG495C.font_chunk_name), 0)
            == 0xffffffffu) {
        si = 0;
    } else {
        game_fread(&DG3890.font_table_34[si], 1, 1, di);

        if (DG3890.font_table_34[si] == 0xfd
            || DG3890.font_table_34[si] == 0xff) {
            uint32_t r;

            DG6176.kind[si] =
                (uint8_t)(-(int8_t)DG3890.font_table_34[si]);

            game_fread(&DG3890.font_table_34[si], 1, 1, di);
            game_fread(&DG3890.font_table_48[si], 1, 1, di);
            game_fread(&DG627A.underline_row[si], 1, 1, di);
            game_fread(&DG3890.font_table_5c[si], 1, 1, di);
            game_fread(&DG3890.font_table_70[si], 1, 1, di);
            game_fread((uint8_t *)size, 1, 2, di);

            r = file_record_size(di);
            handle = open_resource(0xffff, di, 0x4963, r);  /* "r" */
            failed = (handle < 0) ? 1 : 0;

            if (failed == 0)
                failed = ((uint16_t)resource_size(handle) == (uint16_t)size[0])
                         ? 0 : 1;

            if (failed == 0) {
                blk = dos_alloc_bytes((uint16_t)size[0], 0, 0).ptr;
                failed = far_eq(blk, FAR_NULL) ? 1 : 0;
            }

            if (failed == 0)
                failed = (read_resource(handle, MK_FP(blk.seg, blk.off),
                                        (uint16_t)size[0]) == size[0])
                         ? 0 : 1;

            if (failed == 0) {
                /* Three pointers into the one block, the offset stepped
                   two bytes and then one per glyph. The segment does not
                   move, which is why this steps a half rather than the
                   pair. */
                WIDTHSLOT[si] = blk;

                blk.off = (uint16_t)(blk.off
                                     + 2 * DG3890.font_table_70[si]);
                MIDSLOT[si] = blk;

                blk.off = (uint16_t)(blk.off + DG3890.font_table_70[si]);
                FONTSLOT[si] = blk;
            }

            close_resource(handle);

            if (failed != 0) {
                if (!far_eq(blk, FAR_NULL))
                    dos_free_far(blk);
                si = 0;
            }
        } else {
            int16_t glyph_bytes;

            if (DG3890.font_table_34[si] == 0xfe) {
                DG6176.kind[si] = 2;
                game_fread(&DG3890.font_table_34[si], 1, 1, di);
                glyph_bytes = (int16_t)DG3890.font_table_34[si];
            } else {
                DG6176.kind[si] = 0;
                glyph_bytes =
                    (int16_t)((int16_t)(DG3890.font_table_34[si] + 7) >> 3);
            }
            size[0] = glyph_bytes;

            game_fread(&DG3890.font_table_48[si], 1, 1, di);
            game_fread(&DG3890.font_table_5c[si], 1, 1, di);
            game_fread(&DG3890.font_table_70[si], 1, 1, di);

            size[0] = (int16_t)(size[0]
                * (int16_t)((int16_t)DG3890.font_table_48[si]
                            * (int16_t)DG3890.font_table_70[si]));

            p = dg_off(dgroup, heap_malloc_far((uint16_t)size[0]));
            failed = (p == 0) ? 1 : 0;

            if (failed == 0)
                game_fread(dg_ptr(dgroup, p), (uint16_t)size[0], 1, di);

            if (failed == 0) {
                FONTSLOT[si].seg = DGROUP_SEG;
                FONTSLOT[si].off = p;
                WIDTHSLOT[si].seg = 0;
                WIDTHSLOT[si].off = 0;
                MIDSLOT[si].seg = 0;
                MIDSLOT[si].off = 0;
            } else {
                if (p != 0)
                    heap_free_far(dg_ptr(dgroup, p));
                si = 0;
            }
        }
    }

    if (opened != 0)
        close_file_record(di);
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
uint16_t load_bitmap_list(char *name)
{
    struct far_ptr walk;  /* [bp-0xa], [bp-8] - huge_add_to steps it */
    uint16_t count_at;    /* [bp-0x12] */
    bmp_ptr_t *list_at;   /* [bp-2], the array itself now */
    int16_t size_at;   /* [bp-0x16] */

    FILE *si = (FILE *)name;          /* a handle, or a name to open */
    uint16_t opened = 0;                        /* [bp-0x18] */
    int16_t kind = 0;                           /* [bp-0x1a] */
    struct far_ptr blk = FAR_NULL;              /* [bp-4], [bp-6]    */
    struct far_ptr tmp = FAR_NULL;              /* [bp-0xc], [bp-0xe] */
    uint16_t scratch = 0;                       /* [bp-0x10] */
    uint32_t want;                              /* [bp-0x1e], [bp-0x1c] */
    int16_t got;                                /* [bp-0x14] */
    int16_t di = 0;
    uint32_t r;

    list_at = NULL;

    if (file_record_valid(si) == 0) {
        opened = 1;
        si = open_file_record(name);
        /* `or ax,ax` then `jae`: the failure jump here is never taken. */
    }

    if (read_bmp_info(si, &count_at, &list_at) == 0)
        goto done;

    r = vm_bitmap_list_size(dg_off(dgroup, list_at),
                            (uint8_t *)&size_at);
    want = r;

    /* `r` carries a *size* above and an address here; the union is why this
       takes `.ptr` rather than pretending they are one type. */
    blk = dos_alloc_bytes(want, 0, 0).ptr;

    if (far_eq(blk, FAR_NULL))
        goto done;

    if ((uint16_t)size_at != 0) {
        int32_t n = size_at;              /* the `cwd` sign-extends it */

        tmp = dos_alloc_bytes(n, 0, 0).ptr;
    }

    if (far_eq(DG3576.scratch, FAR_NULL)) {
        scratch = dg_off(dgroup, heap_malloc_far(0x3cc4));
        if (scratch != 0) {
            heap_free_far(dg_ptr(dgroup, scratch));
            scratch = dg_off(dgroup, heap_malloc_far(0x3ac4));
            if (scratch != 0) {
                DG3576.scratch.seg = DGROUP_SEG;
                DG3576.scratch.off = scratch;
                huge_add_to(&DG3576.scratch, 0x10);
                DG3576.scratch = normalise_far_ptr_far(
                    (struct far_ptr){
                        (uint16_t)(DG3576.scratch.off & 0xfff0),
                        DG3576.scratch.seg });
            }
        }
    }

    if (seek_named_chunk(si, CHUNK.bmp_bin, 0) == 0xffffffffu)
        goto done;

    r = file_record_size(si);
    di = open_resource(0, si, 0x4978, r);
    if (di < 0)
        goto done;

    walk = blk;

    while (read_resource(di, MK_FP(walk.seg, walk.off),
                         0x7fff) == 0x7fff)
        huge_add_to(&walk, 0x7fff);

    r = resource_size(di);
    vm_load_bitmap_list(list_at, blk, r);

    close_resource(di);
    kind = 1;

    if (DG3890.unknown_1f == 0)
        goto done;

    if (seek_named_chunk(si, CHUNK.bmp_vga, 0) != 0xffffffffu)
        kind = 5;
    if (seek_named_chunk(si, CHUNK.bmp_amg, 0) != 0xffffffffu)
        kind = 6;

    if (kind < 5)
        goto done;

    r = file_record_size(si);
    di = open_resource(0, si, 0x498c, r);
    if (di < 0)
        goto done;

    want = 0x7fff;

    for (;;) {
        {
            struct far_ptr t = dos_alloc_bytes(want, 0, 0).ptr;

            tmp = t;
            if (!far_eq(t, FAR_NULL))
                break;
        }
        /* Halve the request. The original shifts the high word with `sar`,
           so this is a signed 32-bit shift; it starts at 0x7fff and stays
           positive, but the transcription is the shift it makes. */
        want = (uint32_t)((int32_t)want >> 1);
    }

    walk = blk;

    while ((got = read_resource(di, MK_FP(tmp.seg, tmp.off),
                               (uint16_t)want)) > 0) {
        if (kind == 6) {
            expand_1bpp_to_4bpp(tmp, tmp,
                                (uint16_t)got);
            got = (int16_t)(got << 2);
        }

        vm_nothing();       /* vector 0x4382, with five words pushed at it */

        huge_add_to(&walk, (int32_t)(want << 1));
    }

    close_resource(di);

done:
    if (huge_equal(tmp.off, tmp.seg, 0, 0) == 0)
        dos_free_far(tmp);

    if (scratch != 0) {
        heap_free_far(dg_ptr(dgroup, scratch));
        DG3576.scratch.seg = 0;
        DG3576.scratch.off = 0;
    }

    if (kind == 0) {
        if (huge_equal(blk.off, blk.seg, 0, 0) == 0)
            dos_free_far(blk);

        if (di != 0)
            close_resource(di);

        free_bitmap_list(list_at);
        list_at = NULL;
    }

    if (opened != 0)
        close_file_record(si);

    {
        uint16_t answer = dg_off(dgroup, list_at);
        return answer;
    }
}

/*
 * 0x23a18
 *
 * Give back a bitmap list: the block its first word points at, and then the
 * list itself.
 *
 * **The first read is through an unchecked pointer, and that is the
 * original.** 0x23a1f is `cmp word ptr [si], 0` before 0x23a2d tests `si`
 * itself, so the list is dereferenced before it is known to be there. Offset 0
 * is `dgroup` rather than a C null pointer, so a caller's `BMPLIST(0)` reads
 * the same two bytes the original would; only a literal `NULL` would differ,
 * and the callers hand over a list they have already tested.
 */
void free_bitmap_list(bmp_ptr_t * list)
{
    if (list[0] != 0)
        heap_free_far(dg_ptr(dgroup, list[0]));

    if (dg_off(dgroup, list) != 0)
        heap_free_far((uint8_t *)list);
}

/*
 * 0x23a3c
 *
 * Give back everything a bitmap list owns: the block its first header points
 * at, and then the list itself through `free_bitmap_list`.
 *
 * **It does not walk the list, and it does not need to.** 0x23a48 is
 * `mov di, [si]` - the first entry and no other, with no loop anywhere in the
 * routine - because a whole list is *three* allocations rather than two per
 * bitmap, and this pair of routines gives back exactly those three:
 *
 *   the list      `read_bmp_info`: `heap_calloc_far((count + 1) * 2, 1)`,
 *                 count words and a null - freed by `free_bitmap_list`
 *   the headers   `read_bmp_info`: `heap_calloc_far(0xa, count)`, one run of
 *                 `struct bitmap`, and `list[0]` is its first byte - which is
 *                 why `free_bitmap_list` frees `list[0]` as a heap block
 *   the pixels    one `dos_alloc_bytes` for every bitmap in the list, and the
 *                 first header's `data` is its base - which is what this
 *                 routine frees
 *
 * All three loaders agree about that last one: `vm_load_bitmap_list` starts at
 * the block it was handed and steps `off` per bitmap, so header 0 keeps the
 * base; `decode_vqt_list`'s branch of `load_bitmaps` sets `fp2 = block` before
 * its loop. The "BMP:OFF:" branch is the one that would not hold - header 0's
 * data is `block + offsets[0]` out of the file - and it is unreachable with
 * this game's data, for the reasons counted beside `draw_offset_bitmap`.
 *
 * The far pointer is read out of the header the way `vm_load_bitmap_list` wrote
 * it - segment at +0 and offset at +2 - and the `cwd` and `adc` around that read
 * are a 32-bit expression the compiler emitted and then had no use for: `cwd`
 * sets DX and the next instruction clears it, and the `adc` adds a carry that
 * `add dx, [di+2]` cannot produce. Transcribed as the two words it reads.
 */
void free_bitmaps(bmp_ptr_t * list)
{
    if (dg_off(dgroup, list) == 0)
        return;

    dos_free_far(far_of_rev(BMP_PTR(list[0])->data));

    free_bitmap_list(list);
}

/*
 * 0x23a6a
 *
 * How many entries a null-terminated list of near pointers has. A null list is
 * zero rather than a fault.
 */
uint16_t count_list_entries(bmp_ptr_t * list)
{
    uint16_t n = 0;

    if (dg_off(dgroup, list) == 0)
        return 0;

    while (list[n] != 0)
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
 * place: `huge_add_to` takes the address *of* the pointer. That used to mean
 * real DGROUP addresses; since it takes a pointer the two slots are a C
 * array.
 */
void expand_1bpp_to_4bpp(struct far_ptr src, struct far_ptr dst,
                         uint16_t count)
{
    /* Pairs rather than pointers: `huge_add_to` and `huge_sub_from` step both
       and renormalise as they go, which is the four bytes the original
       reserves at [bp+6] and [bp+0xa] for exactly that. Taken by value, so
       the caller's copy is untouched - the original's are its own arguments. */
    int16_t di = (int16_t)count;

    huge_add_to(&src, (uint16_t)(di - 1));
    huge_add_to(&dst, (uint16_t)(di * 4 - 1));

    while (di != 0) {
        int16_t byte;
        int16_t si;

        byte = (int16_t)(int8_t)FAR8(src.seg, src.off);
        huge_sub_from(&src, 1);

        for (si = 1; (si & 0xff) != 0; si = (int16_t)(si << 1)) {
            uint16_t seg = dst.seg;
            uint16_t off = dst.off;

            if ((si & 0xaa) != 0) {
                FAR8(seg, off) = (uint8_t)(FAR8(seg, off)
                                           | ((si & byte) ? 0x10 : 0x00));
                huge_sub_from(&dst, 1);
            } else {
                FAR8(seg, off) = (uint8_t)((si & byte) ? 0x01 : 0x00);
            }
        }

        di--;
    }
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
uint16_t load_screen_plain(char *name)
{
    FILE *handle = (FILE *)name;         /* a handle, or a name to open */
    int16_t w_at[8];          /* [bp-0x10] */
    int16_t h_at;          /* [bp-0x12] */

    uint16_t opened = 0;                         /* [bp-4]  */
    uint16_t kind = 0;                           /* [bp-6]  */
    int16_t res = 0;                             /* [bp-2]  */
    struct far_ptr buf = FAR_NULL;               /* [bp-0xe], [bp-0xc] */
    uint16_t bytes;                              /* [bp-8]  */
    uint16_t half;                               /* [bp-0x14] */
    uint16_t band;                               /* [bp-0xa] */
    int16_t si, di;
    uint32_t r;

    w_at[0] = 0x140;
    h_at = 0xc8;

    /*
     * 0x23b3c is `push cs / call 0x1e94c` - `restore_write_mode`, not
     * `vm_reset_attributes`. The two are both "put the VGA back", which is how
     * the wrong one got written here, and the mistake is invisible on screen:
     * resetting the attribute controller to the identity palette it already
     * holds changes no pixel. The verifier saw it at once - the original's
     * first eight events are the graphics controller and sequencer, the port's
     * were thirty writes to 0x3c0.
     */
    restore_write_mode();

    if (file_record_valid(handle) == 0) {
        opened = 1;
        handle = open_file_record(name);
    }

    if (seek_named_chunk(handle, CHUNK.scr_dim, 0) != 0xffffffffu) {
        game_fread((uint8_t *)w_at, 1, 2, handle);
        game_fread((uint8_t *)&h_at, 1, 2, handle);
    }

    if (seek_named_chunk(handle, CHUNK.scr_bin, 0) == 0xffffffffu)
        goto close;

    r = file_record_size(handle);
    res = open_resource(0, handle, 0x49a0, r);
    if (res < 0)
        goto close;

    half = (uint16_t)(w_at[0] >> 1);
    bytes = (uint16_t)(half << 7);

    do {
        buf.off = dg_off(dgroup, heap_malloc_far(bytes));
        buf.seg = DGROUP_SEG;
        /* The offset alone: it is the heap handle the allocator answered,
           and the segment beside it is always DGROUP's. */
        if (buf.off != 0)
            break;
        bytes = (uint16_t)(bytes >> 1);
    } while (bytes >= half);

    if (buf.off == 0)
        goto close_resource_only;

    di = 0;
    si = (int16_t)(bytes / half);
    band = bytes;
    if (si > h_at)
        si = h_at;

    while (di < h_at) {
        read_resource(res, MK_FP(buf.seg, buf.off), band);
        blit_rows_thunk(buf, 0, di,
                        (int16_t)(half << 1), si);

        di = (int16_t)(di + si);
        if ((int16_t)(di + si) > h_at) {
            si = (int16_t)(h_at - di);
            band = (uint16_t)(si * half);
        }
    }

    kind = 1;

    if (DG3890.unknown_1f == 0)
        goto free_buf;

    close_resource(res);

    if (seek_named_chunk(handle, CHUNK.scr_vga, 0) != 0xffffffffu)
        kind = 5;
    else if (seek_named_chunk(handle, CHUNK.scr_amg, 0) != 0xffffffffu)
        kind = 6;

    if (kind < 5)
        goto free_buf;

    r = file_record_size(handle);
    res = open_resource(0, handle, 0x49b4, r);
    if (res < 0)
        goto free_buf;

    di = 0;
    si = (int16_t)(bytes / half);
    if (kind == 6)
        bytes = (uint16_t)(bytes >> 2);
    band = bytes;
    if (si > h_at)
        si = h_at;

    while (di < h_at) {
        read_resource(res, MK_FP(buf.seg, buf.off), band);

        if (kind == 6)
            expand_1bpp_to_4bpp(buf,
                                buf, band);

        blit_rows_alt_thunk();

        di = (int16_t)(di + si);
        if ((int16_t)(di + si) > h_at) {
            si = (int16_t)(h_at - di);
            band = (uint16_t)(si * half);
            if (kind == 6)
                band = (uint16_t)(band >> 2);
        }
    }

free_buf:
    heap_free_far(dg_ptr(dgroup, buf.off));

close_resource_only:
    close_resource(res);

close:
    if (opened != 0)
        close_file_record(handle);
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
uint16_t find_file_record(FILE *handle)
{
    int16_t i;

    for (i = 3; i >= 0; i--) {
        uint16_t rec = (uint16_t)(0x6292 + 0x43 * i);

        if (FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr) == handle)
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
    if (DG3890.clip_enabled != 0) {
        if (x < DG3890.clip_left)
            return -1;
        if (x > DG3890.clip_right)
            return -1;
        if (y < DG3890.clip_top)
            return -1;
        if (y > DG3890.clip_bottom)
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
uint32_t file_record_size(FILE *handle)
{
    uint16_t rec;

    if (handle == 0)
        return 0xffffffffu;

    rec = find_file_record(handle);
    if (rec == 0)
        return 0xffffffffu;

    return OPENFILE_PTR(rec)->size;
}

/*
 * 0x24308
 *
 * Whether a handle names an open file: 1 or 0. `find_file_record` answers the
 * record and this throws it away, which is the whole routine.
 */
int16_t file_record_valid(FILE *handle)
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
int16_t close_file_record(FILE *handle)
{
    uint16_t rec;

    if (handle == 0)
        return 0;

    rec = find_file_record(handle);
    if (rec == 0)
        return 0;

    OPENFILE_PTR(rec)->file_ptr = 0;
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
    uint16_t handle = OPENFILE_PTR(rec)->file_ptr;
    uint32_t keep = OPENFILE_PTR(rec)->bound[0];
    uint8_t *bytes = dg_ptr(dgroup, rec);
    int16_t i;

    for (i = 0; i < 0x43; i++)
        bytes[i] = 0;

    OPENFILE_PTR(rec)->bound[0] = keep;
    OPENFILE_PTR(rec)->file_ptr = (int16_t)handle;

    game_rewind(FILEREC_PTR(handle));
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
FILE *open_file_record(char *name)
{
    uint16_t rec = find_file_record(0);
    int32_t size;

    if (rec == 0)
        return 0;

    OPENFILE_PTR(rec)->file_ptr = dg_off(dgroup, game_fopen(name, "rb"));
    if (OPENFILE_PTR(rec)->file_ptr == 0)
        return 0;

    game_fseek(FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr), 0, 2);
    size = game_ftell(FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr));

    OPENFILE_PTR(rec)->bound[0] = (uint32_t)size | 0x80000000u;

    reset_file_record(rec);
    return FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr);
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
int16_t string_equal_upto(const char * a, const char * b, uint16_t n)
{
    for (;;) {
        if (*a == 0 && *b == 0)
            return 1;
        if (n == 0)
            return 1;
        n--;

        if (*a != *b)
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
uint8_t * copy_file_record(uint8_t * dst, FILE *handle)
{
    uint16_t rec;

    if (handle == 0 || dst == NULL)
        return NULL;

    rec = find_file_record(handle);
    if (rec == 0)
        return NULL;

    far_move(dg_ptr(dgroup, rec), dst, 0x43);
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
    far_move(DG639E.record, dg_ptr(dgroup, rec), 0x43);
    game_fseek(FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr), (int32_t)OPENFILE_PTR(rec)->pos, 0);
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
uint32_t seek_named_chunk(FILE *handle, const char * path,
                          int16_t index)
{
    uint16_t si;
    int16_t di = 0;
    int16_t keep;

    if (handle == 0)
        return 0xffffffffu;

    si = find_file_record(handle);
    if (si == 0)
        return 0xffffffffu;

    while (path[di] != 0)
        di++;

    if (di == 0 || (di & 3) != 0)
        return 0xffffffffu;

    far_move(dg_ptr(dgroup, si), DG639E.record, 0x43);

    /* The record's own copy of the path walked so far, at +2. It is reached
       through a cast because `OPENFILE` is `volatile` - the record is guest
       memory another routine writes - and an argument is not. */
    if (string_equal_upto(path, (const char *)OPENFILE_PTR(si)->path,
                          0x19) != 0) {
        if (index == 0) {
            int32_t pos = game_ftell(FILEREC_PTR(OPENFILE_PTR(si)->file_ptr));

            if ((uint32_t)pos == OPENFILE_PTR(si)->pos)
                goto at_position;
        }

        if (index == -1) {
            game_fseek(FILEREC_PTR(OPENFILE_PTR(si)->file_ptr), (int32_t)OPENFILE_PTR(si)->pos, 0);
            goto at_position;
        }

        if (OPENFILE_PTR(si)->word_39 != 0) {
            if (index != 0) {
                keep = index;
                if (OPENFILE_PTR(si)->word_39 < index) {
                    index = (int16_t)(index - OPENFILE_PTR(si)->word_39);
                } else if (OPENFILE_PTR(si)->word_39 == index) {
                    game_fseek(FILEREC_PTR(OPENFILE_PTR(si)->file_ptr), (int32_t)OPENFILE_PTR(si)->pos, 0);
                    goto at_position;
                } else {
                    reset_file_record(si);
                }
            } else {
                index = 1;
                keep = (int16_t)(OPENFILE_PTR(si)->word_39 + 1);
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
        uint16_t bx = (uint16_t)(((OPENFILE_PTR(si)->depth >> 2) << 2) & 0xffff);

        if ((OPENFILE_PTR(si)->bound[bx >> 2] & 0x80000000u) == 0) {
            OPENFILE_PTR(si)->pos += OPENFILE_PTR(si)->size;
        }

        game_fseek(FILEREC_PTR(OPENFILE_PTR(si)->file_ptr), (int32_t)OPENFILE_PTR(si)->pos, 0);
    }

    for (;;) {
        /* 0x24290 - one match gone by. */
        if (index-- == 0)
            break;

        for (;;) {
            uint16_t bx = (uint16_t)(((OPENFILE_PTR(si)->depth >> 2) << 2) & 0xffff);

            /* 0x24136 - has this chunk run out? */
            if ((OPENFILE_PTR(si)->bound[bx >> 2] & 0x7fffffffu)
                    == OPENFILE_PTR(si)->pos) {
                if (OPENFILE_PTR(si)->depth == 0)
                    return restore_file_record(si);
                OPENFILE_PTR(si)->depth = (int16_t)(OPENFILE_PTR(si)->depth - 4);
                continue;
            }

            if ((OPENFILE_PTR(si)->bound[bx >> 2] & 0x80000000u) == 0) {
                OPENFILE_PTR(si)->pos += OPENFILE_PTR(si)->size;
                game_fseek(FILEREC_PTR(OPENFILE_PTR(si)->file_ptr), (int32_t)OPENFILE_PTR(si)->pos, 0);
                continue;
            }

            /* 0x241aa - descend into a container. */
            if (game_fread(&OPENFILE_PTR(si)->path[OPENFILE_PTR(si)->depth], 1, 4,
                           FILEREC_PTR(OPENFILE_PTR(si)->file_ptr)) != 4)
                return restore_file_record(si);

            OPENFILE_PTR(si)->depth = (int16_t)(OPENFILE_PTR(si)->depth + 4);
            if (OPENFILE_PTR(si)->depth >= 0x18)
                return restore_file_record(si);

            OPENFILE_PTR(si)->path[OPENFILE_PTR(si)->depth] = 0;

            OPENFILE_PTR(si)->pos += 8;

            if (game_fread(dg_ptr(dgroup, (uint16_t)(si + 0x3f)), 4, 1,
                       FILEREC_PTR(OPENFILE_PTR(si)->file_ptr)) != 1)
                return restore_file_record(si);

            {
                uint32_t end = OPENFILE_PTR(si)->pos + OPENFILE_PTR(si)->size;

                bx = (uint16_t)(((OPENFILE_PTR(si)->depth >> 2) << 2) & 0xffff);
                OPENFILE_PTR(si)->bound[bx >> 2] = end;
            }

            /* Bit 15 of the size's high word is the container flag, and
               is taken off here rather than masked at every read. */
            OPENFILE_PTR(si)->size &= 0x7fffffffu;

            if ((int32_t)OPENFILE_PTR(si)->size < 0)
                return restore_file_record(si);

            {
                /* The outermost bound, with its container flag masked off. */
                uint32_t top = OPENFILE_PTR(si)->bound[0] & 0x7fffffffu;

                if (OPENFILE_PTR(si)->size >= top)
                    return restore_file_record(si);
            }

            if (OPENFILE_PTR(si)->depth != di)
                continue;

            if (string_equal_upto((const char *)OPENFILE_PTR(si)->path, path,
                                  (uint16_t)di) != 0)
                break;
        }
    }

    OPENFILE_PTR(si)->word_39 = keep;

at_position:
    return OPENFILE_PTR(si)->pos;
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
    if (*MK_FP(0xf000, 0xfffe) == 0xff
        && *MK_FP(0xf000, 0xc000) == 0x21)
        DG3890.unknown_1c = 1;

    return (int16_t)(int8_t)DG3890.unknown_1c;
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

    if (DG44EE.installed != 0)
        return 0;

    DG44EE.slot_mask = 0;
    detect_pcjr();

    v = dos_getvect(8);
    S1CS.old_int8.off = (int16_t)v;
    S1CS.old_int8.seg = (int16_t)(v >> 16);

    if (rate > 0xff || rate == 0)
        return 0;

    DG44EE.divider_reload = (int16_t)rate;
    DG44EE.divider = (int16_t)rate;

    divisor = (uint16_t)(0xffffu / rate);
    DG44EE.word_44f1 = (int16_t)divisor;

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

    DG44EE.installed = 1;
    return 1;
}

/*
 * 0x233ef
 *
 * Close one of the ten slots in the table at DGROUP 0x618a, which
 * `table_618a_in_use` answers for. A slot that is not in use is left alone.
 *
 * **The slot that matches entry 0 takes the driver's own state down with it**:
 * six bytes and three pairs of words are cleared, including two that belong to
 * the video driver's data at 0x38ec and 0x38c4. That happens only when the
 * slot's pointer equals entry 0's, so entry 0 is the one the rest hang off.
 *
 * Either way the slot itself is freed - through `dos_free_far` when it has a
 * far pointer at 0x61da and through `heap_free_far` when it does not - and its
 * three table entries and its byte at 0x6176 are cleared.
 */
void close_table_618a_slot(int16_t index)
{
    if (table_618a_in_use(index) == 0)
        return;

    if (far_eq(FONTSLOT[index], DG618A.fonts)) {
        DG6176.kind[0] = 0;
        DG3890.font_table_70[0] = 0;
        DG3890.font_table_5c[0] = 0;
        DG627A.underline_row[0] = 0;
        DG3890.font_table_48[0] = 0;
        DG3890.font_table_34[0] = 0;

        DG61DA.widths = FAR_NULL;
        DG622A.slot    = FAR_NULL;
        DG618A.fonts   = FAR_NULL;
    }

    if (!far_eq(WIDTHSLOT[index], FAR_NULL))
        dos_free_far(WIDTHSLOT[index]);
    else
        heap_free_far(dg_ptr(dgroup, FONTSLOT[index].off));

    DG6176.kind[index] = 0;

    /* The three slot tables, cleared through the types that name them -
       which is what `bx = 4 * index` was computing an offset into. */
    FONTSLOT[index]  = FAR_NULL;
    WIDTHSLOT[index] = FAR_NULL;
    MIDSLOT[index]   = FAR_NULL;
}

/*
 * 0x21158
 *
 * **Take the keyboard back**, the other half of `install_keyboard` above.
 *
 * DGROUP 0x458c is the same flag the install sets; a call with it already
 * clear does nothing and answers 0. Otherwise the BIOS ring is emptied by
 * copying its tail over its head - 0040:001C into 0040:001A - and vectors 09h
 * and 1Ch are put back from the two the install kept in this module's own code
 * segment at 0x4e3c and 0x4e40.
 *
 * The `push ds` / `pop ds` around the two INT 21h calls is because AH=25h
 * takes the handler in DS:DX and DS has to be restored afterwards; the port
 * hands `dos_setvect` the pair and there is nothing to save.
 */
int16_t remove_keyboard(void)
{
    if (DG458C.word_458c == 0)
        return 0;

    DG458C.word_458c = 0;

    FAR16(0x40, 0x1A) = FAR16(0x40, 0x1C);

    dos_setvect(0x09, (uint16_t)S1CS.word_4e3c, (uint16_t)S1CS.word_4e3e);
    dos_setvect(0x1c, (uint16_t)S1CS.word_4e40, (uint16_t)S1CS.word_4e42);

    return 1;
}

/*
 * 0x220cd
 *
 * **Let the mouse go.** The flag at DGROUP 0x48ea says the driver was taken
 * over; clearing it, INT 33h AX=0 resets the driver and AX=0x0C with ES:DX
 * zero takes the event handler off it. Answers 1 if it did the work.
 */
int16_t remove_mouse(void)
{
    if (DG48DA.mouse_taken == 0)
        return 0;

    DG48DA.mouse_taken = 0;

    io_mouse_reset();
    io_mouse_set_handler(0, 0, 0);

    return 1;
}

/*
 * 0x223f7
 *
 * **A restore that restores nothing**, and it is the original's and not a
 * transcription slip.
 *
 * The flag at DGROUP 0x48ec is cleared, and then two pairs of instructions
 * that look like they put vector 0 back - `mov ax,[0x48ef]` then
 * `mov ax,es:[0]`, and the same for [0x48ed] and es:[2] - are **both loads**.
 * The bytes are `a1 ef 48 26 a1 00 00`; a store would be `26 a3`. So the saved
 * words are read into AX and thrown away, and the vector at 0000:0000 is read
 * and thrown away too. Nothing in memory changes.
 *
 * Written out as the flag clear it is, with the loads left off because a load
 * into a register nothing reads is not something C can express and not
 * something anything can observe.
 */
void restore_int0_vector(void)
{
    if (DG48DA.vector_hooked == 0)
        return;

    DG48DA.vector_hooked = 0;
}

/*
 * 0x22741
 *
 * **Back to text.** The two bits at 4 and 5 of the BIOS equipment word at
 * 0040:0010 are set from the argument - that is the "initial video mode" the
 * BIOS boots with - and INT 10h AX=0x0003 puts the adapter in mode 3.
 *
 * BX is loaded with 3 as well, which mode 3 has no use for. Transcribed as the
 * mode set it is.
 *
 * Near and cdecl: `ret` with the argument at [bp+4].
 */
void set_bios_video_mode(uint16_t equipment_bits)
{
    uint8_t eq = FAR8(0x40, 0x10);

    FAR8(0x40, 0x10) = (uint8_t)((eq & 0xcf)
                                 | (uint8_t)((equipment_bits << 4) & 0x30));

    io_bios_set_mode(3);
}

/*
 * 0x225a5
 *
 * The four things that have to be handed back before the program can leave:
 * the keyboard, the mouse, the timer and vector 0. Four calls and nothing else.
 */
void shutdown_input(void)
{
    remove_keyboard();
    remove_mouse();
    timer_remove();
    restore_int0_vector();
}

/*
 * 0x225ba
 *
 * Put the adapter back in the mode the program found it in. DGROUP 0x48f2 is
 * that mode, and 0xff means it was never recorded - which is why the store of
 * 0xff afterwards is inside the test and not after it: having restored the
 * mode, the record is spent.
 */
void restore_video_mode(void)
{
    uint16_t mode = DG48DA.mode_found;

    if (mode != 0xff) {
        set_bios_video_mode(mode);
        DG48DA.mode_found = 0xff;
    }
}

/*
 * 0x1ebdc
 *
 * Free one far block from the table of ten at DGROUP 0x3a2e, found by its
 * address rather than by an index: the pair passed in is compared against each
 * entry and the one that matches is freed and zeroed.
 *
 * Entry 0 is skipped - the walk starts at 1 - and a null argument does nothing
 * at all.
 */
void free_far_block(struct far_ptr h)
{
    int16_t i;

    if (far_eq(h, FAR_NULL))
        return;

    for (i = 1; i < 10; i++) {
        if (!far_eq(DG3A2C.blocks[i], h))
            continue;

        dos_free_far(DG3A2C.blocks[i]);
        DG3A2C.blocks[i] = FAR_NULL;
    }
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
    if (DG44EE.installed == 0)
        return 0;

    io_out8(0x43, 0x36);
    io_out8(0x40, 0);
    io_out8(0x40, 0);
    io_out8(0x21, (uint8_t)(io_in8(0x21) & 0xfc));

    dos_setvect(8, (uint16_t)((int16_t)S1CS.old_int8.off), (uint16_t)((int16_t)S1CS.old_int8.seg));

    DG44EE.installed = 0;
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
void save_rect_thunk(struct far_ptr buf, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_save_rect(buf, x, y, w, h);
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
void restore_rect_thunk(struct far_ptr buf, int16_t x,
                        int16_t y, int16_t w, int16_t h)
{
    vm_restore_rect(buf, x, y, w, h);
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
    return (uint16_t)((*MK_FP(0x40, 0x10) & 0x30) >> 4);
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
    if (DG48DA.mouse_taken == 0)
        return 0;

    DG4740.word_4740 = (int16_t)(x << 2);
    DG4740.word_4742 = (int16_t)(y << 2);

    /*
     * `mov ax,4 / int 0x33` - the driver's "set cursor position", with the
     * quartered coordinates already in CX and DX. The port had the two DGROUP
     * words and not the call, so the pointer was never actually warped.
     */
    io_mouse_move_to((uint16_t)(x << 2), (uint16_t)(y << 2));

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
uint32_t huge_add_positive(struct far_ptr p, uint16_t lo, uint16_t hi)
{
    uint32_t sum = (uint32_t)p.off + lo;
    uint16_t seg = p.seg;

    if (sum > 0xffff)
        seg = (uint16_t)(seg + 0x1000);

    /* **`lo`/`hi` are left as two words on purpose.** Everything else in
       this sweep that looked like a split `long` was one; this is not shown
       to be. The low half is added to the offset and the *high* half alone
       becomes a paragraph count, `(hi >> 5) | ((hi & 0xf) << 12)` - which a
       32-bit `delta >> 4` does not produce, since that would take its low
       bits from `lo`. Nothing in the port calls this routine and
       `verify.py` has never reached it, so nothing can settle which reading
       is right; splitting it into a `uint32_t` would be a guess dressed as a
       cleanup. */
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
    DG48DA.vector_hooked = 1;

    DG48DA.vector.off = (int16_t)*(uint16_t *)(guest_mem + 0);
    DG48DA.vector.seg = (int16_t)*(uint16_t *)(guest_mem + 2);

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
int16_t restore_file_record_from(const uint8_t * src)
{
    uint16_t rec;

    if (src == NULL || (uint16_t)*(int16_t *)(src) == 0)
        return 0;

    rec = find_file_record(FILEREC_PTR((uint16_t)*(int16_t *)(src)));
    if (rec == 0)
        return 0;

    far_move(src, dg_ptr(dgroup, rec), 0x43);
    game_fseek(FILEREC_PTR(OPENFILE_PTR(rec)->file_ptr), (int32_t)OPENFILE_PTR(rec)->pos, 0);
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

    if (far_eq(FONTSLOT[index], FAR_NULL))
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
    uint8_t  entering = DG3890.unknown_00;
    int16_t  index    = (int16_t)(c - DG3890.font_table_5c[0]);
    uint16_t w, h;
    /* The glyph's bytes, walked and never stored - so a pointer, and the
       segment that does not move stops being carried alongside. */
    const uint8_t far *glyph;
    uint16_t row, col;
    int32_t  clipped;
    uint8_t  mask, pixel;
    int32_t  one_bit;

    if (index < 0)
        return 0;
    if ((int16_t)DG3890.font_table_70[0] <= index)
        return 0;

    if (DG6176.kind[0] & 1) {
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
        w = FAR8(DG622A.slot.seg, (uint16_t)(DG622A.slot.off + index));
        h = DG3890.font_table_48[0];
        glyph = MK_FP(DG618A.fonts.seg,
                      (uint16_t)(DG618A.fonts.off
                                 + FARU16(DG61DA.widths.seg,
                                          (uint16_t)(DG61DA.widths.off
                                                     + 2 * index))));
    } else {
        uint16_t units;

        w = DG3890.font_table_34[0];
        h = DG3890.font_table_48[0];
        units = (DG6176.kind[0] == 2) ? (uint16_t)(index * w)
                                   : (uint16_t)(((w + 7) >> 3) * index);
        glyph = MK_FP(DG618A.fonts.seg,
                      (uint16_t)(DG618A.fonts.off + units * h));
    }

    clipped = (x < DG3890.clip_left)
              || (y < DG3890.clip_top)
              || ((uint16_t)(x + w) > ((uint16_t)DG3890.clip_right))
              || ((uint16_t)(y + h) > ((uint16_t)DG3890.clip_bottom));

    one_bit = DG6176.kind[0] <= 1;

    if (DG3890.unknown_02 & 4)
        x = (int16_t)(x + h / 2);

    for (row = 0; row < h; row++) {
        if ((DG3890.unknown_02 & 1) == 0) {
            DG3890.second_colour = DG3890.unknown_01;
            clip_and_draw_line(x, y, (int16_t)(x + w), y);
        }

        mask = 0x80;
        for (col = 0; col < w; col++) {
            int16_t px;

            if (one_bit) {
                if (mask == 0) {
                    mask = 0x80;
                    glyph++;
                }
                pixel = (uint8_t)(*glyph & mask);
                mask = (uint8_t)(mask >> 1);
            } else {
                pixel = *glyph;
                if (pixel != 0)
                    DG3890.unknown_00 = (pixel < 5)
                                  ? DG471E.colour[pixel]
                                  : pixel;
                if ((uint16_t)(w - 1) > col)
                    glyph++;
            }

            px = (int16_t)(x + col);

            if (pixel != 0) {
                if ((DG3890.unknown_02 & 0x10) && (((px + y) & 1) == 0)) {
                    /* half-tone: this one is skipped, but bold still draws */
                    if (DG3890.unknown_02 & 2)
                        draw_char_plot(clipped, (int16_t)(px + 1), y,
                                       (int16_t)DG3890.unknown_00);
                } else {
                    draw_char_plot(clipped, px, y, (int16_t)DG3890.unknown_00);
                    if ((DG3890.unknown_02 & 0x10) == 0 && (DG3890.unknown_02 & 2))
                        draw_char_plot(clipped, (int16_t)(px + 1), y,
                                       (int16_t)DG3890.unknown_00);
                }
            } else if ((DG3890.unknown_02 & 8) && DG627A.underline_row[0] == row) {
                draw_char_plot(clipped, px, y, (int16_t)entering);
            }
        }

        if ((DG3890.unknown_02 & 4) && (row & 1))
            x--;

        y++;
        glyph++;
    }

    DG3890.unknown_00 = entering;
    return w;
}

/*
 * 0x218eb
 *
 * **Draw a string.** The body, and it takes a **far** pointer; 0x218d4 below is
 * the door that puts `ds` in front of a caller's near one. The picker's listing
 * is what needs the far form - its text is in a block DOS handed over, not in
 * DGROUP - and a null is both halves being zero, not just the offset.
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
void draw_string_body(const char far *str, int16_t x, int16_t y)
{
    uint16_t w;

    /* The original tests `(str | seg) == 0` - a far pointer of 0000:0000,
       which is not a C null pointer but the first byte of guest memory. */
    if (str == (const char far *)MK_FP(0, 0))
        return;

    /*
     * The three tests are not all the same kind. The style at 0x3892 is
     * compared with `jle` - **signed**, so a style byte with bit 7 set passes
     * it - the clip flag at 0x3893 is sign-extended with `cbw` before being
     * tested against zero, and the font marker at 0x6176 is compared with
     * `jbe`, unsigned. Written as three unsigned tests they would agree on
     * every value this game uses and disagree on a style of 0x80 or more.
     */
    if ((int8_t)DG3890.unknown_02 <= 1 && (int8_t)DG3890.clip_enabled == 0
        && DG6176.kind[0] <= 1) {
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
        w = DG3890.font_table_34[0];

        while (*str != 0) {
            int16_t  index;
            uint16_t h;
            const uint8_t far *glyph;

            if (w > 8) {
                x = (int16_t)(x + draw_char(*str, x, y));
                str++;
                continue;
            }

            index = (int16_t)(*str - DG3890.font_table_5c[0]);

            if (!far_eq(DG61DA.widths, FAR_NULL)) {
                /* Far pointers, as in `draw_char`; see the note there. */
                w = FAR8(DG622A.slot.seg, (uint16_t)(DG622A.slot.off + index));
                h = DG3890.font_table_48[0];
                glyph = MK_FP(DG618A.fonts.seg,
                              (uint16_t)(DG618A.fonts.off
                                  + FARU16(DG61DA.widths.seg,
                                           (uint16_t)(DG61DA.widths.off
                                                      + 2 * index))));
            } else {
                uint16_t stride;

                w = DG3890.font_table_34[0];
                h = DG3890.font_table_48[0];
                stride = (uint16_t)((w + 7) >> 3);
                glyph = MK_FP(DG618A.fonts.seg,
                              (uint16_t)(DG618A.fonts.off
                                         + stride * h * index));
            }

            vm_blit_glyph(glyph, w, h, x, y);
            x = (int16_t)(x + w);
            str++;
        }
        return;
    }

    while (*str != 0) {
        w = draw_char(*str, x, y);

        x = (int16_t)(x + w);
        if (DG3890.unknown_02 & 2)
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
void draw_string(const char *str, int16_t x, int16_t y)
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
uint16_t text_width(const char *str)
{
    uint16_t width = 0;
    int16_t  proportional = (DG61DA.widths.off | DG61DA.widths.seg) != 0;

    while (*str != 0) {
        int16_t index = (int16_t)((uint8_t)*str - DG3890.font_table_5c[0]);

        str++;
        if (index < 0)
            break;
        if ((int16_t)DG3890.font_table_70[0] <= index)
            break;

        /* `les bx, [0x622a]`: the width table is far. See `draw_char`. */
        width = (uint16_t)(width + (proportional
                                    ? FAR8(DG622A.slot.seg,
                                           (uint16_t)(DG622A.slot.off + index))
                                    : DG3890.font_table_34[0]));
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

    return DG3890.font_table_48[slot];
}

/*
 * 0x215ff
 *
 * `text_width`, reached the way every caller reaches it: the string arrives as
 * a near offset and the body wants a far pointer, so this pushes `ds` in front
 * of it and calls through. Nothing else.
 */
uint16_t text_width_thunk(const char *str)
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
uint16_t read_bmp_info(FILE *handle, uint16_t * count_at,
                       bmp_ptr_t ** out)
{
    uint16_t tmp = 0;
    uint16_t rows;
    /* The list this routine allocates, and a cursor along the run of headers
       it allocates beside it. The guest keeps the list as one word in the
       caller's `[bp-2]`; the port hands back the array itself, and `off` is
       the near pointer the allocator answered, kept because the cleanup path
       frees by offset. */
    dg_off_t off = 0;
    bmp_ptr_t *list = NULL;
    bmp_ptr_t di;
    int16_t *a, *b;
    int16_t i;

    *out = NULL;

    if (seek_named_chunk(handle, CHUNK.bmp_inf, 0) == 0xffffffffu)
        return 0;

    if (game_fread((uint8_t *)count_at, 2, 1, handle) != 1)
        return 0;

    off = heap_calloc_far((uint16_t)((*count_at + 1) * 2), 1);
    if (off == 0)
        goto cleanup;

    /* One run of headers for the whole list, and `list[0]` is its first
       byte - which is why `free_bitmap_list` gives it back as a heap block. */
    list = BMPLIST(off);
    *out = list;
    list[0] = heap_calloc_far(sizeof(struct bitmap), *count_at);
    if (list[0] == 0)
        goto cleanup;

    {
        uint32_t sz = file_record_size(handle) - 2;
        uint32_t need = (uint32_t)(int16_t)
                        (*count_at * 4);

        rows = (sz >= need) ? *count_at : 1;
    }

    tmp = dg_off(dgroup, heap_malloc_far((uint16_t)(rows * 4)));
    if (tmp == 0)
        goto cleanup;

    if (game_fread(dg_ptr(dgroup, tmp), (uint16_t)(rows * 4), 1, handle) != 1)
        goto cleanup;

    /* `rows` widths then `rows` heights, straight out of the file. A typed
       pointer is safe over this one where it would not be over a packed
       record: `tmp` is a near-heap block and every block address is even,
       because the low bit of the size word beside it is the in-use flag. */
    a = (int16_t *)dg_ptr(dgroup, tmp);
    b = a + rows;
    di = list[0];

    for (i = 0; *count_at > i; i++) {
        list[i] = di;
        BMP_PTR(di)->width = *a;
        BMP_PTR(di)->height = *b;

        /* Only when there is a row per bitmap; otherwise every header takes
           the same pair, which is what `rows = 1` above means. */
        if (*count_at == rows) {
            a++;
            b++;
        }

        di = (bmp_ptr_t)(di + sizeof(struct bitmap));
    }

    /* The null. With `*count_at` of zero the loop does not run and this puts
       it over `list[0]`, which is what the original's cursor does too. */
    list[i] = 0;
    heap_free_far(dg_ptr(dgroup, tmp));
    return 1;

cleanup:
    if (tmp != 0)
        heap_free_far(dg_ptr(dgroup, tmp));

    if (off != 0) {
        if (list[0] != 0)
            heap_free_far(dg_ptr(dgroup, list[0]));
        heap_free_far(dg_ptr(dgroup, off));
    }

    *out = NULL;
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
    uint8_t al = DG48DA.mode_forced;

    if (DG4342.word_4344 == 0)
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

    al = DG48DA.mode_forced;
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
uint32_t load_video_driver(int16_t adapter, char *name)
{
    FILE *file = (FILE *)name;         /* a handle, or a name to open */
    uint16_t opened = 0;
    FILE *di;
    int16_t handle;
    uint32_t len;
    int16_t si = adapter;

    switch (adapter) {
    case 4:
        si = 1;                           /* overwritten below, never read */
        DG3F78.screen_width = 0x280;
        si = 8;
        DG3F78.screen_height = 0x190;
        break;
    case 0xc:
        si = 0xb;
        DG3F78.screen_height = 0x15e;
        break;
    case 0xd:
        si = 0xb;
        DG3F78.screen_height = 0x1e0;
        break;
    case 0xe:
        si = 0xb;
        DG3F78.screen_height = 0x190;
        break;
    case 0xf:
        si = 8;
        DG3F78.screen_height = 0x190;
        break;
    default:
        break;
    }

    if (file_record_valid(file) == 0) {
        opened = 1;
        di = open_file_record(name);
    } else {
        di = file;
    }

    if (di == 0)
        return 0;

    /* `si` runs from 1 here, and 0x48ff is `0x4901 - 2` - the compiler
       folding that first index into the base, so entry 0 is not a tag. */
    string_copy_far(dg_off(dgroup, OVLCHUNK.ovl_tag + 4),
                    ADAPTER_TAGS[si]);

    if (seek_named_chunk(di, OVLCHUNK.ovl_tag, 0) == 0xffffffffu)
        return 0;

    {
        uint32_t sz = file_record_size(di);

        handle = open_resource(0xffff, di, 0x495a, sz);
    }

    if (handle < 0)
        return 0;

    {
        uint32_t sz = resource_size(handle);

        len = sz;
    }

    if (!huge_equal(DG48F8.block.off, DG48F8.block.seg, 0, 0))
        dos_free_far(DG48F8.block);

    {
        struct far_ptr p = dos_alloc_bytes(len, 0, 0).ptr;

        DG48F8.block = p;
    }

    if (huge_equal(DG48F8.block.off, DG48F8.block.seg, 0, 0))
        return 0;

    read_resource(handle, MK_FP(DG48F8.block.seg, DG48F8.block.off),
                  (uint16_t)len);
    close_resource(handle);

    if (opened != 0)
        close_file_record(di);

    return ((uint32_t)DG48F8.block.seg << 16) | DG48F8.block.off;
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
 * is the BIOS "get font pointer" call and it answers in **ES:BP**, so the two
 * pairs the game files here are that answer and not, as this note once said,
 * the routine's own frame pointer.
 *
 * Nothing implements it on either side. The emulator leaves the registers as
 * it found them, so it answers `0000:ffca` - which is BP, which is the frame
 * pointer, which is why the mistake was easy to make - and the port answers a
 * plain zero, because a font pointer aimed at the stack is an accident rather
 * than a behaviour to reproduce. Real fonts are a separate piece of work.
 *
 * So the two sides differ by four bytes at DGROUP 0x618a and 0x618e, on
 * purpose. `vm_init`'s spec carries `deviation` so a sweep says so, and
 * STATUS.md records it.
 */
uint16_t vm_init(uint16_t adapter, uint16_t unused, FILE *file)
{
    /*
     * **No frame.** The prologue at 0x22483 is `push bp / mov bp,sp / push si
     * / push di` with no `sub sp` at all: the four bytes are SI and DI, and
     * saving registers is not something the port has to model.
     *
     * The font pointer below is not this routine's BP either, however much it
     * looks like it - see `io_bios_font_ptr`.
     */
    struct bios_font_ptr font;
    uint16_t al;
    uint16_t r;

    (void)unused;

    DG48DA.mode_forced = (uint8_t)adapter;
    DG3F78.mode_kind = 0;
    DG3890.unknown_1f = 0;
    DG3F78.screen_width = 0x140;
    DG3F78.screen_height = 0xc8;

    if (!far_eq(DG3A2C.blocks[0], FAR_NULL)) {
        dos_free_far(DG3A2C.blocks[0]);
        DG3A2C.blocks[0] = FAR_NULL;
    }

    DG48DA.mode_found = (uint8_t)bios_video_kind();

    al = detect_adapter() & 0xff;
    DG3890.pixel_shift = (uint8_t)al;

    if (al != 0) {
        uint32_t p = load_video_driver((int16_t)al, (char *)file);

        if ((uint16_t)(p >> 16) == 0) {
            DG3890.pixel_shift = 0;
        } else {
            uint16_t seg;
            int16_t i;

            DG48DA.driver.off = (int16_t)p;
            DG48DA.driver.seg = (int16_t)(p >> 16);

            vm_driver_init(0x3890, 0x4412, DGROUP_SEG);
            seg = DG48DA.driver.seg;

            /* a hundred words of the driver's table, word by word, and
               then the driver's segment over every second one */
            for (i = 0; i < 0x64; i++)
                ((int16_t *)DG4342.font)[i] =
                    *(int16_t *)MK_FP(seg, (uint16_t)(0x13e + 2 * i));

            for (i = 0; i < 0x32; i++)
                DG4342.font[i].seg = seg;
        }
    } else {
        DG3890.pixel_shift = 0;
    }

    *(uint16_t *)(guest_mem + 0x4f0) = DGROUP_SEG;

    DG3890.page_src_ptr = ((int16_t)DG3890.page_front_ptr);
    DG3890.page_dst_ptr = ((int16_t)DG3890.page_back_ptr);

    r = ((uint8_t)DG3890.pixel_shift);
    if (r == 0)
        goto out;

    if (DG4342.word_4342 != 0)
        dos_free_far((struct far_ptr){ 0, (uint16_t)(DG4342.word_4342 - 1) });

    {
        struct far_ptr p = dos_alloc_bytes((uint16_t)(((uint16_t)DG3F78.screen_height) * 4 + 0x20), 0, 0).ptr;

        if (p.seg == 0)
            goto out;

        DG4342.word_4342 = (int16_t)(p.seg + 1);
    }

    /*
     * `mov ax,0x1130 / mov bh,3 / int 0x10`, and the answer is in **ES:BP** -
     * so the four words are that pair, filed twice. The emulator does not
     * implement the call, which is why they come back zero; see
     * `io_bios_font_ptr`.
     */
    font = io_bios_font_ptr(3);

    DG618A.fonts.off = (int16_t)font.bp;
    DG618A.fonts.seg = (int16_t)font.es;
    DG618A.bios_fonts.off = (int16_t)font.bp;
    DG618A.bios_fonts.seg = (int16_t)font.es;

    *(int16_t *)(&DG3890.font_table_48[0]) = 0x808;
    *(int16_t *)(&DG3890.font_table_34[0]) = 0x808;
    *(int16_t *)(&DG3890.font_table_5c[0]) = 0;
    *(int16_t *)(&DG3890.font_table_70[0]) = (int16_t)0xffff;

out:
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
 * A **** routine: its first argument is at [bp+4], not [bp+6].
 */
void planes_to_chunky(uint8_t far * dst, const uint8_t far * src,
                      uint16_t count)
{
    /* Four plane cursors into one block, `count` apart. Only ever read
       through, so pointers - and the 16-bit wrap the original's
       `src_off + 3 * count` has is given up here, which is the standing
       trade for the `far` tag: the four planes are one allocation and
       cannot straddle a segment. */
    const uint8_t far * p0 = src;
    const uint8_t far * p1 = src + count;
    const uint8_t far * p2 = src + 2 * count;
    const uint8_t far * p3 = src + 3 * count;
    uint8_t mask = 0x80;

    while (count != 0) {
        uint8_t v = 0;

        if ((*p0 & mask) != 0) v = (uint8_t)(v | 1);
        if ((*p1 & mask) != 0) v = (uint8_t)(v | 2);
        if ((*p2 & mask) != 0) v = (uint8_t)(v | 4);
        if ((*p3 & mask) != 0) v = (uint8_t)(v | 8);

        *dst = v;
        dst++;

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
    uint16_t first = BMPSET_PTR(list)->bmp[0];
    uint16_t segs;
    uint16_t over;

    DG63E2.mode = (uint8_t)(colours - 1);
    DG63E2.word_63f2 = dg_off(dgroup, heap_malloc_far(0x7d0));

    /* The first bitmap's own pixels, which is where the output begins. Its
       header stores the pair segment-first. */
    DG63E2.out_start = far_of_rev(BMP_PTR(first)->data);
    DG63E2.out = DG63E2.out_start;

    while (BMPSET_PTR(si)->bmp[0] != 0) {
        uint16_t hdr = BMPSET_PTR(si)->bmp[0];
        uint16_t di = DG63E2.out.off;
        struct far_ptr at;

        /* Normalise, and remember where this bitmap's own data begins. The
           shift is *signed*, which is the original's `sar`. */
        at.seg = (uint16_t)(DG63E2.out.seg + (uint16_t)((int16_t)di >> 4));
        at.off = (uint16_t)(di & 0x0f);
        DG63E2.out = at;

        if (DG3890.unknown_1f == 0) {
            uint16_t pixels = (uint16_t)(BMP_PTR(hdr)->width
                                         * BMP_PTR(hdr)->height);
            struct far_ptr blk = dos_alloc_bytes(pixels, 0, 0).ptr;


            pixels = (uint16_t)(pixels >> 3);

            planes_to_chunky(MK_FP(blk.seg, blk.off),
                             MK_FP(BMP_PTR(hdr)->data.seg, BMP_PTR(hdr)->data.off),
                             pixels);

            BMP_PTR(hdr)->data = far_to_rev(blk);

            compress_bitmap(si);

            dos_free_far(blk);
        } else {
            compress_bitmap(si);
        }

        hdr = BMPSET_PTR(si)->bmp[0];
        BMP_PTR(hdr)->data = far_to_rev(at);
        BMP_PTR(hdr)->mask_off = 0xfffe;

        si = (uint16_t)(si + 2);
    }

    segs = (uint16_t)(DG63E2.out.seg - DG63E2.out_start.seg);
    over = (uint16_t)(DG63E2.out.off - DG63E2.out_start.off);
    DG63E2.word_63e8 = (uint16_t)(segs + (uint16_t)((int16_t)(over + 0x0f) >> 4));

    io_dos_resize(BMP_PTR(BMPSET_PTR(list)->bmp[0])->data.seg, DG63E2.word_63e8);

    heap_free_far(dg_ptr(dgroup, DG63E2.word_63f2));

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
 * A **** routine: its argument is at [bp+4].
 */
void emit_packed_value(int16_t value)
{
    int16_t dx = value;

    if (DG63E2.pending_rows != 0) {
        if (dx < 0) {
            dx = (int16_t)(-dx);

            FAR8(DG63E2.out.seg, DG63E2.out.off) = (uint8_t)(dx & 0x3f);
            DG63E2.out.off++;

            dx = (int16_t)((dx & 0x1c0) >> 6);

            if (dx != 0) {
                FAR8(DG63E2.out.seg, DG63E2.out.off) = (uint8_t)(dx & 0x3f);
                DG63E2.out.off++;
            }

            while (--DG63E2.pending_rows != 0) {
                FAR8(DG63E2.out.seg, DG63E2.out.off) = 0;
                DG63E2.out.off++;
            }
            return;
        }

        while (DG63E2.pending_rows-- != 0) {
            FAR8(DG63E2.out.seg, DG63E2.out.off) = 0;
            DG63E2.out.off++;
        }
        DG63E2.pending_rows = 0;
    }

    while (dx > 0x3f) {
        FAR8(DG63E2.out.seg, DG63E2.out.off) = 0x7f;
        DG63E2.out.off++;
        dx = (int16_t)(dx - 0x3f);
    }

    FAR8(DG63E2.out.seg, DG63E2.out.off) = (uint8_t)(0x40 | (dx & 0xff));
    DG63E2.out.off++;
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
 * pixels. A **** routine: its arguments are at [bp+4] and [bp+6].
 */
void write_literal_run(uint8_t count, const uint8_t * buf)
{
    uint8_t dl = count;
    int16_t si;

    FAR8(DG63E2.out.seg, DG63E2.out.off) = (uint8_t)(dl | 0xc0);
    DG63E2.out.off++;

    if ((dl & 1) != 0) {
        ((uint8_t *)buf)[dl] = 0;
        dl++;
    }

    if (((uint8_t)DG63E2.mode) == 0x0f) {
        for (si = 0; (int16_t)dl > si; si += 2) {
            uint8_t v = (uint8_t)((buf[si] << 4)
                                  | buf[si + 1]);

            FAR8(DG63E2.out.seg, DG63E2.out.off) = v;
            DG63E2.out.off++;
        }
    } else {
        for (si = 0; (int16_t)dl > si; si++) {
            FAR8(DG63E2.out.seg, DG63E2.out.off) = buf[si];
            DG63E2.out.off++;
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
 * A **** routine, and it walks its own `remaining` argument down - which
 * nothing can see, because the caller pops it.
 */
void compress_row(uint16_t src, int16_t remaining)
{
    uint8_t buf[260];                  /* [bp-0x104], 0x101 bytes */

    const uint8_t *di = dg_ptr(dgroup, src);
    uint8_t literals = 0;               /* [bp-3] */
    uint8_t run = 0;                    /* [bp-2] */
    uint8_t value = 0;                  /* [bp-1] */

    while (remaining > 0) {
        const uint8_t *si = di;

        run = 1;
        value = *si;
        si++;
        while (*si == value) {
            si++;
            run++;
        }

        if ((int16_t)run >= DG49BA.min_run) {
            if ((int16_t)run > remaining)
                run = (uint8_t)remaining;

            if (literals != 0) {
                write_literal_run(literals, buf);
                literals = 0;
            }

            remaining = (int16_t)(remaining - run);
            di += run;

            while (run > 0x3f) {
                run = (uint8_t)(run + 0xc1);        /* less 0x3f */
                FAR8(DG63E2.out.seg, DG63E2.out.off) = 0xbf;
                DG63E2.out.off++;
                FAR8(DG63E2.out.seg, DG63E2.out.off) = value;
                DG63E2.out.off++;
            }

            if (run != 0) {
                FAR8(DG63E2.out.seg, DG63E2.out.off) = (uint8_t)(0x80 | run);
                DG63E2.out.off++;
                FAR8(DG63E2.out.seg, DG63E2.out.off) = value;
                DG63E2.out.off++;
            }
            run = 0;
        } else {
            remaining--;
            buf[literals] = value;
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
 * A **** routine.
 */
void compress_bitmap(uint16_t header)
{
    uint8_t rowbuf[334];               /* [bp-0x14e] */

    uint16_t si = header;
    uint16_t di = 0;                    /* pixels waiting in the row buffer */
    int16_t blanks = 0;                 /* [bp-6], and it does go negative */
    uint8_t least = 0xff;               /* [bp-7] */
    struct far_ptr hdr;
    int16_t x, y;

    DG63E2.pending_rows = 0;
    DG63E2.word_63e8 = 0;

    DG63E2.word_63ec = BMP_PTR(si)->data.seg;
    DG63E2.word_63ea = BMP_PTR(si)->data.off;

    if (((uint8_t)DG63E2.mode) == 0x0f && DG3890.unknown_1f != 0) {
        for (y = 0; BMP_PTR(si)->height > y; y++)
            for (x = 0; BMP_PTR(si)->width > x; x++) {
                uint8_t v = FAR8(DG63E2.word_63ec, DG63E2.word_63ea);

                DG63E2.word_63ea++;
                if (v != 0 && v < least)
                    least = v;
            }
    } else {
        least = 1;
    }

    DG63E2.word_63ec = BMP_PTR(si)->data.seg;
    DG63E2.word_63ea = BMP_PTR(si)->data.off;

    hdr = DG63E2.out;
    DG63E2.out.off++;

    for (y = 0; BMP_PTR(si)->height > y; y++) {
        uint8_t *at = rowbuf;

        far_memcpy((uint8_t *)rowbuf,
                   MK_FP((uint16_t)DG63E2.word_63ec,
                           (uint16_t)DG63E2.word_63ea),
                   (uint16_t)BMP_PTR(si)->width);
        DG63E2.word_63ea = (uint16_t)(DG63E2.word_63ea + BMP_PTR(si)->width);

        for (x = 0; BMP_PTR(si)->width > x; x++) {
            uint8_t v = (*at);

            at++;

            if (v == 0) {
                if (di != 0) {
                    compress_row(DG63E2.word_63f2, (int16_t)di);
                    di = 0;
                }
                blanks++;
                continue;
            }

            v = (uint8_t)((v - least) & ((uint8_t)DG63E2.mode));
            dg_ptr(dgroup, DG63E2.word_63f2)[di] = v;
            di++;

            if (blanks != 0) {
                emit_packed_value(blanks);
                blanks = 0;
            } else if (DG63E2.pending_rows != 0) {
                while (DG63E2.pending_rows-- != 0) {
                    FAR8(DG63E2.out.seg, DG63E2.out.off) = 0;
                    DG63E2.out.off++;
                }
                DG63E2.pending_rows = 0;
            }
        }

        if (di != 0) {
            compress_row(DG63E2.word_63f2, (int16_t)di);
            di = 0;
        }

        blanks = (int16_t)(blanks - BMP_PTR(si)->width);
        DG63E2.pending_rows++;
    }

    if (di != 0)
        compress_row(DG63E2.word_63f2, (int16_t)di);

    emit_packed_value(0);

    FAR8(hdr.seg, hdr.off) = least;
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
int16_t compute_step(uint8_t * rec, int16_t count)
{
    int32_t span;
    int32_t step;
    int32_t was_negative = 0;

    if (count <= 0) {
        *(int16_t *)(rec + 6) = 0;
        *(int16_t *)(rec + 4) = 0;
        *(int16_t *)(rec) = 0;
        return 0;
    }

    *(int16_t *)(rec) = 0;
    *(int16_t *)(rec + 4) = 0;

    span = (int32_t)(((uint32_t)(uint16_t)*(int16_t *)(rec + 6) << 16))
         - (int32_t)(((uint32_t)(uint16_t)*(int16_t *)(rec + 2) << 16));

    step = long_divide(span, (int32_t)count);

    if (step < 0) {
        step = -step;
        was_negative = 1;
    }

    *(int16_t *)(rec + 6) = (int16_t)(step >> 16);
    *(int16_t *)(rec + 4) = (int16_t)step;

    *(int16_t *)(rec) = (step == 0) ? (int16_t)0x8000 : (int16_t)step;

    if (was_negative) {
        step = -step;
        *(int16_t *)(rec + 6) = (int16_t)(step >> 16);
        *(int16_t *)(rec + 4) = (int16_t)step;
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
 * A **** routine - `ret`, not `retf` - so its argument is at bp+4 and not
 * bp+6. The scaled blitter calls it seven times.
 */
int16_t scale_table_delta(int16_t n)
{
    uint16_t base = DG628E.base;

    return (int16_t)(SCALE_TABLE[(base + n)]
                     - SCALE_TABLE[base]);
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
static void step_accumulate(uint8_t * rec)
{
    uint32_t acc = ((uint32_t)(uint16_t)*(int16_t *)(rec + 2) << 16)
                 | (uint16_t)*(int16_t *)(rec);
    uint32_t step = ((uint32_t)(uint16_t)*(int16_t *)(rec + 6) << 16)
                  | (uint16_t)*(int16_t *)(rec + 4);

    acc += step;

    *(int16_t *)(rec) = (int16_t)acc;
    *(int16_t *)(rec + 2) = (int16_t)(acc >> 16);
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
 * `x + DG3890.clip_right` at 0x22ab4 - `03 06 96 38`, an `add` - where the mirrored
 * *solid* run at 0x22bf3 computes `x - DG3890.clip_right`, `2b 06 96 38`, a `sub`.
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
    uint8_t scratch[320];                        /* [bp-0x172] */
    int16_t vstep32[4];    /* [bp-0x2a], the accumulator */
    int16_t vpage;    /* [bp-0x1e] */
    int16_t vrow;    /* [bp-0x1c] */
    uint8_t vclip;    /* [bp-0x1a] */
    uint8_t vrowok;    /* [bp-0x19] */
    /* A cursor into `scratch`, not storage - see the note on the same slot
       in `draw_compressed_bitmap`. The original keeps it in two frame bytes
       because it has nowhere else; nothing outside the frame reads it. */
    uint8_t * vp;                                /* [bp-0x18] */
    int16_t vcut;    /* [bp-0x16] */
    int16_t vx2;    /* [bp-0x14] */
    int16_t vydir;    /* [bp-0x12] */
    int16_t vcol;    /* [bp-0x10] */
    int16_t vsrc[2];    /* [bp-0xa], offset then seg */
    uint8_t vbase;    /* [bp-0x21] */
    uint8_t vcolour;    /* [bp-0x22] */
    int16_t vn;    /* [bp-4] */
    int16_t vop;    /* [bp-2] */
    int16_t vx0;    /* [bp-0x30] */
    int16_t vxrow;    /* [bp-0x2e] */
    int16_t vcolrow;    /* [bp-0x32] */
    int16_t vrowacc;    /* [bp-0x2c] */
    int16_t vsrcrow[2];    /* [bp-0xe], the row's start */
    int16_t *vrepeat = &vcut;   /* the same slot as `vcut` */    /* [bp-0x16], reused */
    /*
     * [bp-6], and it has to be its own slot. The skipped-row loop at 0x22d94
     * keeps its scaled delta here - `mov [bp-6], ax` at 0x22db0 - while the
     * count of rows still to skip sits in [bp-0x16]. Writing the delta through
     * `vcut`, which *is* [bp-0x16], overwrote the counter with a pixel
     * distance: the loop then skipped as many source rows as the sprite was
     * wide and the decode walked off into the next rows' tags. Every scaled
     * part on the briefing screen came out as a smear.
     */
    int16_t vdelta;    /* [bp-6] */
    int16_t  i, j;

    if (w == 0 || h == 0) {
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
    vpage = (int16_t)DG3890.page_dst_ptr;
    if (DG3F72.page_hook != 0)
        vm_nothing();

    vclip = DG3890.clip_enabled;
    if (vclip != 0
        && x >= DG3890.clip_left && (int16_t)(x + w) <= DG3890.clip_right
        && y >= DG3890.clip_top && (int16_t)(y + h) <= DG3890.clip_bottom)
        vclip = 0;

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
    vstep32[1] = 0;
    vstep32[3] = w;
    compute_step((uint8_t *)vstep32, BMP_PTR(hdr)->width);

    i = 0;
    j = 0;
    while (BMP_PTR(hdr)->width >= i) {
        int16_t at = vstep32[1];

        if (at > w)
            at = w;
        SCALE_TABLE[i] = at;

        step_accumulate((uint8_t *)vstep32);

        while (j < at) {
            ROW_OFFSETS[j] = (uint16_t)(i - 1);
            j++;
        }
        i++;
    }

    vrowacc = 0;

    if (mode & 1) {
        vydir = -1;
        y = (int16_t)(y + h - 1);
    } else {
        vydir = 1;
    }

    if (vclip != 0) {
        vrowok = (y <= DG3890.clip_bottom && y >= DG3890.clip_top) ? 1 : 0;
        if (vrowok != 0)
            vrow = (int16_t)ROW_BASE[y];
    } else {
        vrow = (int16_t)ROW_BASE[y];
    }

    vsrc[1] = (int16_t)BMP_PTR(hdr)->data.seg;              /* the segment */
    vsrc[0] = (int16_t)BMP_PTR(hdr)->data.off;              /* the offset */

    vbase = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
    vsrc[0]++;

    vx0   = x;
    vxrow = x;
    DG628E.base = 0;
    vcolrow = 0;
    DG628E.word_6290 = (uint16_t)SCALE_TABLE[0];

    vsrcrow[0] = (int16_t)vsrc[0];
    vsrcrow[1] = (int16_t)vsrc[1];

    vstep32[1] = 0;
    vstep32[3] = (int16_t)(BMP_PTR(hdr)->height - 1);
    compute_step((uint8_t *)vstep32, (int16_t)(h - 1));

    for (;;) {
        vop = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
        vsrc[0]++;

        if ((vop & 0x80) && (vop & 0x40)) {
            /* 0x22997 - a run of nibbles, decoded into the row buffer. */
            vop &= 0x3f;
            vn = scale_table_delta(vop);

            if (vop != 0) {
                int16_t  at    = SCALE_TABLE[DG628E.base];
                int16_t  first = (int16_t)ROW_OFFSETS[at];
                uint8_t *  out   = scratch;
                int16_t  k     = vn;
                int16_t  col   = at;

                while (k-- > 0) {
                    int16_t rel = (int16_t)((int16_t)ROW_OFFSETS[col] - first);
                    uint16_t byte_at = (uint16_t)((uint16_t)rel >> 1);
                    uint8_t  b = *MK_FP((uint16_t)vsrc[1],
                                          (uint16_t)((uint16_t)vsrc[0] + byte_at));

                    /*
                     * `shr` puts bit 0 in the carry and `jae` takes the even
                     * column, so an even column is the *high* nibble.
                     */
                    *out = (uint8_t)(((rel & 1) ? (b & 0x0f) : (b >> 4))
                                     + vbase);
                    out++;
                    col++;
                }

                vsrc[0] = (int16_t)((uint16_t)vsrc[0]
                                         + ((vop + 1) >> 1));
            }

            DG628E.base = (uint16_t)(DG628E.base + vop);
            if (vn == 0)
                continue;

            vp = scratch;

            if (mode & 2) {
                vx2 = (int16_t)(x - vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_run;
                    if (!(vx2 >= DG3890.clip_left && x < DG3890.clip_right)) {
                        if (vx2 < DG3890.clip_left) {
                            vcut = (int16_t)(DG3890.clip_left - vx2);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                        } else {
                            /* The `add` at 0x22ab4, as written. */
                            vcut = (int16_t)(x + DG3890.clip_right);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                            vp = vp + vcut;
                            x = DG3890.clip_right;
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)vn,
                            vp,
                            (struct far_ptr){ (uint16_t)vrow,
                                              (uint16_t)vpage }, 1);
            } else {
                vx2 = (int16_t)(x + vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_run;
                    if (!(x >= DG3890.clip_left && vx2 <= DG3890.clip_right)) {
                        if (x < DG3890.clip_left) {
                            vcut = (int16_t)(DG3890.clip_left - x);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                            vp = vp + vcut;
                            x = DG3890.clip_left;
                        } else {
                            vcut = (int16_t)(vx2 - DG3890.clip_right - 1);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)vn,
                            vp,
                            (struct far_ptr){ (uint16_t)vrow,
                                              (uint16_t)vpage }, 0);
            }

next_run:
            x = vx2;
            continue;
        }

        if (vop & 0x80) {
            /* 0x22b5b - a solid run: one colour byte, plus the base. */
            vop &= 0x3f;
            vn = scale_table_delta(vop);
            DG628E.base = (uint16_t)(DG628E.base + vop);

            vcolour = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
            vsrc[0]++;

            if (mode & 2) {
                vx2 = (int16_t)(x - vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_solid;
                    if (!(vx2 >= DG3890.clip_left && x < DG3890.clip_right)) {
                        if (vx2 < DG3890.clip_left) {
                            vcut = (int16_t)(DG3890.clip_left - vx2);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                        } else {
                            vcut = (int16_t)(x - DG3890.clip_right);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                            x = DG3890.clip_right;
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(vbase + vcolour),
                        (uint16_t)(x - vn + 1), vn,
                        (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage });
            } else {
                vx2 = (int16_t)(x + vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_solid;
                    if (!(x >= DG3890.clip_left && vx2 <= DG3890.clip_right)) {
                        if (x < DG3890.clip_left) {
                            vcut = (int16_t)(DG3890.clip_left - x);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                            x = (int16_t)(x + vcut);
                        } else {
                            vcut = (int16_t)(vx2 - DG3890.clip_right - 1);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(vcolour + vbase),
                        (uint16_t)x, vn,
                (struct far_ptr){ (uint16_t)vrow, (uint16_t)vpage });
            }

next_solid:
            x = vx2;
            continue;
        }

        if (vop & 0x40) {
            /* 0x22c96 - a move along the row; a count of zero ends it all. */
            vop &= 0x3f;
            if (vop == 0)
                break;

            vn = scale_table_delta(vop);
            DG628E.base = (uint16_t)(DG628E.base + vop);

            if (mode & 2)
                x = (int16_t)(x - vn);
            else
                x = (int16_t)(x + vn);
            continue;
        }

        /* 0x22cc9 - the end of a row. */
        vop &= 0x3f;
        vn = scale_table_delta((int16_t)-vop);
        if (vn < 0)
            vn = (int16_t)-vn;
        DG628E.base = (uint16_t)(DG628E.base - vop);

        if (mode & 2)
            x = (int16_t)(x + vn);
        else
            x = (int16_t)(x - vn);

        /*
         * Peek at the next tag without consuming it: only one with both top
         * bits clear is taken here, as a second move of its low six bits
         * shifted up by six.
         */
        vop = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
        if ((vop & 0xc0) == 0) {
            vcol = (int16_t)(vop & 0x3f);
            if (vcol != 0) {
                vsrc[0]++;
                vcol = (int16_t)(vcol << 6);
                vn = scale_table_delta(vcol);
                DG628E.base = (uint16_t)(DG628E.base - vcol);
                if (mode & 2)
                    x = (int16_t)(x + vn);
                else
                    x = (int16_t)(x - vn);
            }
        }

        /* 0x22d45 - step the row accumulator and see how many rows it covers. */
        step_accumulate((uint8_t *)vstep32);

        vx2 = vstep32[1];

        if (vrowacc == vx2) {
            /*
             * The scaled row lands on the same destination row as the last
             * one, so this source row is not drawn at all: the source pointer,
             * x and the column index all go back to where the row began.
             */
            vsrc[0] = (int16_t)vsrcrow[0];
            vsrc[1] = (int16_t)vsrcrow[1];
            x = vxrow;
            DG628E.base = (uint16_t)vcolrow;
        } else {
            int16_t repeat = (int16_t)(vx2 - vrowacc);

            if (repeat < 0)
                repeat = (int16_t)-repeat;
            repeat--;

            vrepeat[0] = repeat;

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
            while (vrepeat[0] != 0) {
                vop = *MK_FP((uint16_t)vsrc[1], (uint16_t)vsrc[0]);
                vsrc[0]++;
                vn = (int16_t)(vop & 0x3f);
                vdelta = scale_table_delta(vn);
                if (mode & 2)
                    vdelta = (int16_t)-vdelta;

                if (vop & 0x80) {
                    DG628E.base = (uint16_t)(DG628E.base + vn);
                    x = (int16_t)(x + vdelta);
                    if (vop & 0x40)
                        vsrc[0] = (int16_t)((uint16_t)vsrc[0]
                                                 + ((vn + 1) >> 1));
                    else
                        vsrc[0]++;
                } else if (vop & 0x40) {
                    if (vn == 0)
                        goto done;
                    DG628E.base = (uint16_t)(DG628E.base + vn);
                    x = (int16_t)(x + vdelta);
                } else {
                    DG628E.base = (uint16_t)(DG628E.base - vn);
                    x = (int16_t)(x - vdelta);

                    vop = *MK_FP((uint16_t)vsrc[1],
                                         (uint16_t)vsrc[0]);
                    if ((vop & 0xc0) == 0) {
                        vcol = (int16_t)(vop & 0x3f);
                        if (vcol != 0) {
                            vsrc[0]++;
                            vcol = (int16_t)(vcol << 6);
                            vn = scale_table_delta(vcol);
                            DG628E.base =
                                (uint16_t)(DG628E.base - vcol);
                            if (mode & 2)
                                x = (int16_t)(x + vn);
                            else
                                x = (int16_t)(x - vn);
                        }
                    }
                    vrepeat[0] = (int16_t)(vrepeat[0] - 1);
                }
            }
        }

        /* 0x22e73 - the row is finished; remember where the next one begins. */
        vsrcrow[0] = (int16_t)vsrc[0];
        vsrcrow[1] = (int16_t)vsrc[1];
        vrowacc = vx2;
        vxrow   = x;
        vcolrow = (int16_t)DG628E.base;

        h--;
        if (h == 0)
            break;

        {
            int16_t back = SCALE_TABLE[DG628E.base];

            if (mode & 2)
                back = (int16_t)-back;
            x = (int16_t)(vx0 + back);
        }

        y = (int16_t)(y + vydir);

        if (vclip != 0) {
            vrowok = (y <= DG3890.clip_bottom && y >= DG3890.clip_top) ? 1 : 0;
            if (vrowok == 0)
                continue;
        }

        vrow = (int16_t)ROW_BASE[y];
    }

done:
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
    int16_t rec[16];                 /* [bp-0x20], the 16.16 accumulator */
    int16_t  right, bottom, left, top, cut;
    int16_t  stride, plane_size;
    int16_t  i, j, row, want;
    uint16_t off, page;
    struct far_ptr src;

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
        rec[1] = (int16_t)(BMP_PTR(hdr)->width - 1);
        rec[3] = 0;
    } else {
        rec[1] = 0;
        rec[3] = (int16_t)(BMP_PTR(hdr)->width - 1);
    }

    compute_step((uint8_t *)rec, (int16_t)(right - 1));

    for (i = 0; i < right; i++) {
        SCALE_TABLE[i] = rec[1];
        step_accumulate((uint8_t *)rec);
    }

    /* One column of overrun past the end, so the driver's run can read it. */
    SCALE_TABLE[i] =
        (int16_t)(SCALE_TABLE[i] + 1);

    /*
     * The row table, holding each destination row's *byte offset* into the
     * source rather than its row number - accumulated a stride at a time, so
     * the driver needs no multiply. The step always runs forwards; mirroring
     * writes the entries in from the far end instead.
     */
    rec[1] = 0;
    rec[3] = (int16_t)(BMP_PTR(hdr)->height - 1);
    compute_step((uint8_t *)rec, (int16_t)(bottom - 1));

    stride = (int16_t)(BMP_PTR(hdr)->width
                       >> DG457A.stride_shift[(int8_t)((uint8_t)DG3890.pixel_shift)]);
    plane_size = (int16_t)(BMP_PTR(hdr)->height * stride);

    off = 0;
    row = 0;
    for (j = 0; j < bottom; j++) {
        want = rec[1];
        step_accumulate((uint8_t *)rec);

        while (want > row) {
            row++;
            off = (uint16_t)(off + stride);
        }

        if (mode & 1)
            ROW_OFFSETS[bottom - j - 1] = off;   /* `[bx+0x5e54]`, one entry down */
        else
            ROW_OFFSETS[j] = off;
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
    if (DG3890.clip_enabled != 0) {
        if (right > DG3890.clip_right)
            right = (int16_t)(right - (right - DG3890.clip_right - 1));
        if (bottom > DG3890.clip_bottom)
            bottom = (int16_t)(bottom - (bottom - DG3890.clip_bottom - 1));
        if (top < DG3890.clip_top)
            top = DG3890.clip_top;
        if (left < DG3890.clip_left) {
            cut  = (int16_t)(DG3890.clip_left - left);
            left = DG3890.clip_left;
        }
    }

    src = far_of_rev(BMP_PTR(hdr)->data);

    if (bottom - top > 0 && right - left > 1) {
        /*
         * Set/reset off, write mode 0, and the index left on the bit mask -
         * which no other path here does, and which the driver row blit relies
         * on. `restore_write_mode` puts them back.
         */
        if (DG3890.adapter == 0x10) {
            io_out16(PORT_GC_INDEX, 0x0001);
            io_out16(PORT_GC_INDEX, 0x0005);
            io_out8(PORT_GC_INDEX, 0x08);
        }

        page = DG3890.page_dst_ptr;
        if (DG3F72.page_hook != 0)
            vm_nothing();

        for (j = top; j < bottom; j++)
            vm_blit_scaled_row(
                (uint16_t)plane_size,
                &SCALE_TABLE[cut],
                ROW_BASE[j],
                page, left, (int16_t)(right - left),
                (struct far_ptr){
                    (uint16_t)(ROW_OFFSETS[j - y] + src.off),
                    src.seg });

        restore_write_mode();
    }
}

/*
 * 172c:39b7, image 0x20c07
 *
 * Clip the polygon against the window, in two passes: left and right into the
 * working arrays at 0x398c and dg_off(dgroup, DG3890.work_y), then top and bottom back into 0x393c and
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
void clip_polygon(void)
{
    int16_t si, di, bx;
    uint8_t cl, ch;
    int16_t n;

    di = 0;
    n = (int16_t)DG3A2C.clip_count;
    if (n <= 1)
        return;

    bx = (int16_t)((n - 1) * 2);

    cl = 0;
    if (DG3890.poly_x[bx >> 1] < DG3890.clip_left)
        cl |= 1;
    if (DG3890.poly_x[bx >> 1] > DG3890.clip_right)
        cl |= 2;

    for (si = 0; ; ) {
        ch = 0;
        if (DG3890.poly_x[si >> 1] < DG3890.clip_left)
            ch |= 1;
        if (DG3890.poly_x[si >> 1] > DG3890.clip_right)
            ch |= 2;

        if ((cl | ch) == 0) {
            DG3890.work_x[di >> 1] = DG3890.poly_x[si >> 1];
            DG3890.work_y[di >> 1] = DG3890.poly_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* Both outside the same edge: nothing survives. */
        } else if (cl == 0) {
            /* Leaving: the crossing only. */
            int16_t edge = (ch & 1) ? DG3890.clip_left
                         : (ch & 2) ? DG3890.clip_right : 0;

            if (ch & 3) {
                DG3890.work_x[di >> 1] = edge;
                DG3890.work_y[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.poly_y[bx >> 1]
                              - DG3890.poly_y[si >> 1])
                    * (int32_t)(int16_t)(edge - DG3890.poly_x[si >> 1])
                    / (int32_t)(int16_t)(DG3890.poly_x[bx >> 1]
                                         - DG3890.poly_x[si >> 1])
                    + DG3890.poly_y[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            /* Arriving: the crossing, and then the point itself. */
            int16_t edge = (cl & 1) ? DG3890.clip_left
                         : (cl & 2) ? DG3890.clip_right : 0;

            if (cl & 3) {
                DG3890.work_x[di >> 1] = edge;
                DG3890.work_y[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.poly_y[si >> 1]
                              - DG3890.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(edge - DG3890.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(DG3890.poly_x[si >> 1]
                                         - DG3890.poly_x[bx >> 1])
                    + DG3890.poly_y[bx >> 1]);
                di += 2;
            }

            DG3890.work_x[di >> 1] = DG3890.poly_x[si >> 1];
            DG3890.work_y[di >> 1] = DG3890.poly_y[si >> 1];
            di += 2;
        } else {
            /* Out one side and in the other: both crossings, no vertex. */
            int16_t e1 = (cl & 1) ? DG3890.clip_left
                       : (cl & 2) ? DG3890.clip_right : 0;
            int16_t e2 = (ch & 1) ? DG3890.clip_left
                       : (ch & 2) ? DG3890.clip_right : 0;

            if (cl & 3) {
                DG3890.work_x[di >> 1] = e1;
                DG3890.work_y[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.poly_y[si >> 1]
                              - DG3890.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(e1 - DG3890.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(DG3890.poly_x[si >> 1]
                                         - DG3890.poly_x[bx >> 1])
                    + DG3890.poly_y[bx >> 1]);
                di += 2;
            }

            if (ch & 3) {
                DG3890.work_x[di >> 1] = e2;
                DG3890.work_y[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.poly_y[bx >> 1]
                              - DG3890.poly_y[si >> 1])
                    * (int32_t)(int16_t)(e2 - DG3890.poly_x[si >> 1])
                    / (int32_t)(int16_t)(DG3890.poly_x[bx >> 1]
                                         - DG3890.poly_x[si >> 1])
                    + DG3890.poly_y[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)DG3A2C.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    n = (int16_t)((uint16_t)di >> 1);
    DG3A2C.clip_count = (uint16_t)n;

    if (n <= 1) {
        int16_t i;

        for (i = 0; i < n; i++) {
            DG3890.poly_x[i] = ((uint16_t)DG3890.work_x[i]);
            DG3890.poly_y[i] = ((uint16_t)DG3890.work_y[i]);
        }
        return;
    }

    bx = (int16_t)((n - 1) * 2);
    di = 0;

    cl = 0;
    if (DG3890.work_y[bx >> 1] > DG3890.clip_bottom)
        cl |= 4;
    if (DG3890.work_y[bx >> 1] < DG3890.clip_top)
        cl |= 8;

    for (si = 0; ; ) {
        ch = 0;
        if (DG3890.work_y[si >> 1] > DG3890.clip_bottom)
            ch |= 4;
        if (DG3890.work_y[si >> 1] < DG3890.clip_top)
            ch |= 8;

        if ((cl | ch) == 0) {
            DG3890.poly_x[di >> 1] = DG3890.work_x[si >> 1];
            DG3890.poly_y[di >> 1] = DG3890.work_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* nothing */
        } else if (cl == 0) {
            int16_t edge = (ch & 4) ? DG3890.clip_bottom
                         : (ch & 8) ? DG3890.clip_top : 0;

            if (ch & 12) {
                DG3890.poly_y[di >> 1] = edge;
                DG3890.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.work_x[bx >> 1]
                              - DG3890.work_x[si >> 1])
                    * (int32_t)(int16_t)(edge - DG3890.work_y[si >> 1])
                    / (int32_t)(int16_t)(DG3890.work_y[bx >> 1]
                                         - DG3890.work_y[si >> 1])
                    + DG3890.work_x[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            int16_t edge = (cl & 4) ? DG3890.clip_bottom
                         : (cl & 8) ? DG3890.clip_top : 0;

            if (cl & 12) {
                DG3890.poly_y[di >> 1] = edge;
                DG3890.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.work_x[si >> 1]
                              - DG3890.work_x[bx >> 1])
                    * (int32_t)(int16_t)(edge - DG3890.work_y[bx >> 1])
                    / (int32_t)(int16_t)(DG3890.work_y[si >> 1]
                                         - DG3890.work_y[bx >> 1])
                    + DG3890.work_x[bx >> 1]);
                di += 2;
            }

            DG3890.poly_x[di >> 1] = DG3890.work_x[si >> 1];
            DG3890.poly_y[di >> 1] = DG3890.work_y[si >> 1];
            di += 2;
        } else {
            int16_t e1 = (cl & 4) ? DG3890.clip_bottom
                       : (cl & 8) ? DG3890.clip_top : 0;
            int16_t e2 = (ch & 4) ? DG3890.clip_bottom
                       : (ch & 8) ? DG3890.clip_top : 0;

            if (cl & 12) {
                DG3890.poly_y[di >> 1] = e1;
                DG3890.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.work_x[si >> 1]
                              - DG3890.work_x[bx >> 1])
                    * (int32_t)(int16_t)(e1 - DG3890.work_y[bx >> 1])
                    / (int32_t)(int16_t)(DG3890.work_y[si >> 1]
                                         - DG3890.work_y[bx >> 1])
                    + DG3890.work_x[bx >> 1]);
                di += 2;
            }

            if (ch & 12) {
                DG3890.poly_y[di >> 1] = e2;
                DG3890.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(DG3890.work_x[bx >> 1]
                              - DG3890.work_x[si >> 1])
                    * (int32_t)(int16_t)(e2 - DG3890.work_y[si >> 1])
                    / (int32_t)(int16_t)(DG3890.work_y[bx >> 1]
                                         - DG3890.work_y[si >> 1])
                    + DG3890.work_x[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)DG3A2C.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    DG3A2C.clip_count = (uint16_t)((uint16_t)di >> 1);
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
void poly_walk(uint16_t seg, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di)
{
    int16_t di_step = (int8_t)DG44DE.byte_44e8;

    di = (uint16_t)((di << 2) + DG44D0.chain);

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
void poly_edge_vertical(uint16_t seg, int16_t x,
                        int16_t y1, int16_t y2)
{
    if (y2 <= y1) {
        int16_t t = y1;

        y1 = y2;
        y2 = t;
    }

    DG44DE.byte_44e8 = 2;
    poly_walk(seg, x, 0, 0, 0, (int16_t)(y2 - y1 + 1), (uint16_t)y1);
}

/*
 * 172c:316f, image 0x1f3bf - an edge at exactly 45 degrees.
 *
 * One across for every one down, so again no fractional part: the step is 1 or
 * -1 by which way the x runs.
 */
void poly_edge_diagonal(uint16_t seg, int16_t x1, int16_t x2,
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

    DG44DE.byte_44e8 = 2;
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
void poly_edge_steep(uint16_t seg, int16_t x1, int16_t x2,
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

    di = (uint16_t)((y1 << 2) + DG44D0.chain);

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
void poly_edge_shallow_right(uint16_t seg, int16_t x1, int16_t x2,
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
void poly_edge_shallow_left(uint16_t seg, int16_t x1, int16_t x2,
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
void poly_outline(int16_t *xs, int16_t *ys, int16_t n)
{
    if (DG3F78.mode_kind == 0) {
        while (n-- > 0) {
            clip_and_draw_line(xs[0], ys[0],
                               xs[1],
                               ys[1]);
            xs++;
            ys++;
        }
        return;
    }

    DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top >> 1);
    DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom >> 1);

    while (n-- > 0) {
        clip_and_draw_line(xs[0], (int16_t)(ys[0] >> 1),
                           xs[1],
                           (int16_t)(ys[1] >> 1));
        xs++;
        ys++;
    }

    DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top << 1);
    DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom << 1);
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
void draw_polygon(int16_t n, const int16_t *xs, const int16_t *ys)
{
    uint16_t seg;
    int16_t ax, bx, cx, dx, si, di, bp;
    int16_t i;

    DG44DE.word_44e2 = 0;
    DG44DE.byte_44e9 = 0;

    if (n >= 0) {
        DG3A2C.clip_count = (uint16_t)n;
        for (i = 0; i < n; i++) {
            DG3890.poly_x[i] = xs[i];
            DG3890.poly_y[i] = ys[i];
        }
    }

    if (n < 2)
        goto out;

    if (n == 2) {
        poly_outline(DG3890.poly_x, DG3890.poly_y, 1);
        goto out;
    }

    if (DG3890.fill_enabled == 0) {
        /* Filling is off: close the ring and draw it as lines. */
        n = (int16_t)DG3A2C.clip_count;
        DG3890.poly_x[n] = ((uint16_t)DG3890.poly_x[0]);
        DG3890.poly_y[n] = ((uint16_t)DG3890.poly_y[0]);
        poly_outline(DG3890.poly_x, DG3890.poly_y, n);
        goto out;
    }

    if (DG3890.second_colour != DG3890.fill_colour) {
        n = (int16_t)DG3A2C.clip_count;
        DG44DE.word_44e4 = (uint16_t)n;

        for (i = 0; i < n; i++) {
            DG3890.closed_x[i] = ((uint16_t)DG3890.poly_x[i]);
            DG3890.closed_y[i] = ((uint16_t)DG3890.poly_y[i]);
        }
        DG3890.closed_x[n] = ((uint16_t)DG3890.poly_x[0]);
        DG3890.closed_y[n] = ((uint16_t)DG3890.poly_y[0]);
    }

    if (DG3890.clip_enabled != 0)
        clip_polygon();

    n = (int16_t)DG3A2C.clip_count;
    if (n < 2)
        goto out;
    if (n == 2) {
        poly_outline(DG3890.poly_x, DG3890.poly_y, 1);
        goto out;
    }

    si = (int16_t)((n - 1) * 2);
    DG44DE.word_44e0 = ((uint16_t)DG3890.poly_y[0]);
    dx = 0x7fff;
    bx = (int16_t)0x8001;
    DG44DE.word_44de = ((uint16_t)DG3890.poly_x[0]);
    bp = dx;
    cx = bx;
    di = 0;
    DG44D0.word_44d0 = 0;
    DG44D0.word_44d2 = 0;

    for (; si >= 0; si -= 2) {
        ax = DG3890.poly_y[si >> 1];

        if (ax == DG44DE.word_44e0
            && DG3890.poly_x[si >> 1] == DG44DE.word_44de)
            continue;

        DG44DE.word_44e0 = ax;
        DG3890.work_y[di >> 1] = ax;

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
            || (ax == dx && DG3890.poly_x[si >> 1] > cx)) {
            DG44D0.word_44d0 = (uint16_t)di;
            dx = ax;
            cx = DG3890.poly_x[si >> 1];
        }

        if (ax > bx
            || (ax == bx && DG3890.poly_x[si >> 1] <= bp)) {
            DG44D0.word_44d2 = (uint16_t)di;
            bx = ax;
            bp = DG3890.poly_x[si >> 1];
        }

        ax = DG3890.poly_x[si >> 1];
        DG44DE.word_44de = ax;
        DG3890.work_x[di >> 1] = ax;
        di += 2;
    }

    if (dx == bx) {
        /* Every point on one row: one line, and nothing to fill. */
        if (DG3F78.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top >> 1);
            DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top << 1);
            DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom << 1);
        }
        goto out;
    }

    ax = (int16_t)((uint16_t)di >> 1);
    if (ax < 2)
        goto out;

    if (ax == 2) {
        if (DG3F78.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top >> 1);
            DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            DG3890.clip_top = (int16_t)((uint16_t)DG3890.clip_top << 1);
            DG3890.clip_bottom = (int16_t)((uint16_t)DG3890.clip_bottom << 1);
        }
        goto out;
    }

    cx = di;
    DG3A2C.clip_count = (uint16_t)ax;

    /*
     * Which way round is it wound? Compare the slopes of the two edges leaving
     * the top vertex. A zero rise is turned into one with a huge run so the
     * comparison still means something, and the two slopes are compared as
     * quotient-then-remainder rather than by cross-multiplying, because the
     * product would not fit.
     */
    si = (int16_t)DG44D0.word_44d0;
    di = (int16_t)(si + 2);
    if (di >= cx)
        di = 0;

    dx = (int16_t)(DG3890.work_x[di >> 1] - DG3890.work_x[si >> 1]);
    bp = (int16_t)(DG3890.work_y[di >> 1] - DG3890.work_y[si >> 1]);
    if (bp == 0) {
        bp = 1;
        dx = (dx >= 0) ? 0x7fff : (int16_t)-0x7fff;
    }

    di = (int16_t)(si - 2);
    if (di < 0)
        di = (int16_t)(di + cx);

    ax = (int16_t)(DG3890.work_x[di >> 1] - DG3890.work_x[si >> 1]);
    bx = (int16_t)(DG3890.work_y[di >> 1] - DG3890.work_y[si >> 1]);
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
    DG44DE.byte_44e9 = 1;
    DG44DE.word_44e6 = (uint16_t)cx;
    for (i = 0; i < cx; i += 2) {
        DG3890.closed_x[i >> 1] = DG3890.work_x[i >> 1];
        DG3890.closed_y[i >> 1] = DG3890.work_y[i >> 1];
    }

keep:
    for (i = 0; i < cx; i += 2) {
        DG3890.poly_x[i >> 1] = DG3890.work_x[i >> 1];
        DG3890.poly_y[i >> 1] = DG3890.work_y[i >> 1];
    }
    goto chains;

reverse:
    for (i = 0; i < cx; i += 2) {
        DG3890.poly_x[(cx - 2 - i) >> 1] = DG3890.work_x[i >> 1];
        DG3890.poly_y[(cx - 2 - i) >> 1] = DG3890.work_y[i >> 1];
    }
    DG44D0.word_44d0 = (uint16_t)(cx - 2 - (int16_t)DG44D0.word_44d0);
    DG44D0.word_44d2 = (uint16_t)(cx - 2 - (int16_t)DG44D0.word_44d2);

chains:
    /* The right chain: from the bottom vertex up to the top. */
    dx = DG3890.poly_y[DG44D0.word_44d2 >> 1];
    si = (int16_t)DG44D0.word_44d0;
    di = 0;
    for (;;) {
        DG3890.work_x[di >> 1] = DG3890.poly_x[si >> 1];
        ax = DG3890.poly_y[si >> 1];
        DG3890.work_y[di >> 1] = ax;
        di += 2;
        if (ax >= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    DG44D0.word_44d4 = (uint16_t)((uint16_t)di >> 1);

    /* The left chain: from the top vertex down to the bottom. */
    dx = DG3890.poly_y[DG44D0.word_44d0 >> 1];
    si = (int16_t)DG44D0.word_44d2;
    for (;;) {
        DG3890.work_x[di >> 1] = DG3890.poly_x[si >> 1];
        ax = DG3890.poly_y[si >> 1];
        DG3890.work_y[di >> 1] = ax;
        di += 2;
        if (ax <= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    DG44D0.word_44d6 = (uint16_t)(((uint16_t)di >> 1) - DG44D0.word_44d4);

    seg = DG4342.word_4342;

    DG44D0.chain = 2;
    DG44D0.word_44da = 0;
    ax = (int16_t)DG44D0.word_44d4;

    for (;;) {
        ax--;
        if (ax == 0) {
            if (DG44D0.chain != 0) {
                DG44D0.word_44da += 2;
                DG44D0.chain = 0;
                ax = (int16_t)DG44D0.word_44d6;
                continue;
            }
            break;
        }

        DG44D0.word_44d8 = (uint16_t)ax;

        si = (int16_t)DG44D0.word_44da;
        DG44D0.word_44da = (uint16_t)(si + 2);

        {
            int16_t x1 = DG3890.work_x[si >> 1];
            int16_t x2 = DG3890.work_x[(si >> 1) + 1];
            int16_t y1 = DG3890.work_y[si >> 1];
            int16_t y2 = DG3890.work_y[(si >> 1) + 1];
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
                    uint16_t at = (uint16_t)((y1 << 2) + DG44D0.chain);

                    FAR16(seg, at) = (DG44D0.chain == 0) ? lo : hi;
                } else if (adx < ady) {
                    poly_edge_steep(seg, x1, x2, y1, y2);
                } else if (adx > ady) {
                    /* `cmp [0x44dc],0; jne 0x1f3e6; je 0x1f4a1`. */
                    if (DG44D0.chain != 0)
                        poly_edge_shallow_right(seg, x1, x2, y1, y2);
                    else
                        poly_edge_shallow_left(seg, x1, x2, y1, y2);
                } else {
                    poly_edge_diagonal(seg, x1, x2, y1, y2);
                }
            }
        }

        ax = (int16_t)DG44D0.word_44d8;
    }

    /* Hand the whole buffer to the driver's span filler in one call. */
    {
        int16_t top = DG3890.poly_y[DG44D0.word_44d0 >> 1];
        int16_t bottom = DG3890.poly_y[DG44D0.word_44d2 >> 1];
        uint16_t at = (uint16_t)((top << 2) + 0x0c);

        DG44DE.word_44e2 = seg;

        FAR16((uint16_t)(seg - 1), at) = top;
        FAR16((uint16_t)(seg - 1), (uint16_t)(at + 2)) =
            (int16_t)(bottom - top + 1);

        vm_fill_spans(MK_FP((uint16_t)(seg - 1), at));
    }

    if (DG3890.second_colour != DG3890.fill_colour)
        poly_outline(DG3890.closed_x, DG3890.closed_y, (int16_t)DG44DE.word_44e4);

out:
    if (DG44DE.byte_44e9 != 0) {
        /* The second pass, for a polygon whose two top edges had one slope. */
        DG44DE.byte_44e9 = 0;
        cx = (int16_t)DG44DE.word_44e6;
        for (i = 0; i < cx; i += 2) {
            DG3890.work_x[i >> 1] = ((uint16_t)DG3890.closed_x[i >> 1]);
            DG3890.work_y[i >> 1] = ((uint16_t)DG3890.closed_y[i >> 1]);
        }
        goto reverse;
    }
}
