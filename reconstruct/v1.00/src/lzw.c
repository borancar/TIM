/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The LZW decoder and the run-length decoder over memory** - type 2 of
 * the resource handlers, the code reader under it, and type 1 for a
 * resource already in memory. Hand-written assembly: the decoder stops in
 * the middle of a string and resumes there on the next call, with DS moved
 * onto the dictionary; so it is TASM source, the `#ifdef __TURBOC__` block
 * below, with the host's transcription in the `#else`.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1ca46..0x1ce1f - the decoder's own head comes before its entry at
 * 0x1ca62 - with its `_DATA` 0x35b2..0x35d6. Its ends are resource.c's last
 * routine and resfile.c's first.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: structs resource=res engine_stream=strm
 * JUDGE: assembler bc3.00
 */
#include <string.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
INCLUDE STRUCTS.ASH
_DATA segment word public 'DATA'
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh, 0h
lzw_code_buf label byte
        db 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch
lzw_rmask label byte
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh
lzw_resume_src label byte
        db 0h, 0h, 0h
rle_spill_at label byte
        db 0h, 0h
_DATA ends

extrn _g_engine_resource_flags:byte
extrn _g_engine_stream:byte
LZW_TEXT segment byte public 'CODE'
assume cs:LZW_TEXT, ds:DGROUP
extrn _emit_byte:near
extrn _read_input_block:near
public _decompress_lzw, _next_lzw_code, _rle_from_memory
lzw_first_code:
        mov byte ptr DGROUP:_g_engine_stream+strm_first_code, 0
        mov bp, es
        call _next_lzw_code
        mov word ptr DGROUP:_g_engine_stream+strm_oldcode, ax
        mov word ptr DGROUP:_g_engine_stream+strm_finchar, ax
        push ax
        call _emit_byte
        add sp, 2
        mov es, bp
        jmp short lzw_next_code
lzw_pad_90 db 90h

/* 0x1ca62 */
_decompress_lzw proc near
        push bp
        push si
        push di
        mov ax, word ptr DGROUP:_g_engine_stream+strm_scratch+2
        add ax, 372h
        mov es, ax
        cmp byte ptr DGROUP:_g_engine_stream+strm_resume, 0
        je lzw_fresh
        mov cx, word ptr DGROUP:_g_engine_stream+strm_wanted
        inc cx
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        mov si, word ptr DGROUP:lzw_resume_src
        mov byte ptr DGROUP:_g_engine_stream+strm_resume, 0
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je lzw_resume_skip
        jmp lzw_copy_next
lzw_resume_skip:
        jmp lzw_skip_next
lzw_fresh:
        cmp byte ptr DGROUP:_g_engine_stream+strm_first_code, 0
        jne lzw_first_code
lzw_next_code:
        mov bp, es
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jl lzw_end
        cmp ax, 100h
        jne lzw_code
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_scratch
        mov ax, di
        mov cx, 100h
        rep stosw
        inc ax
        mov word ptr DGROUP:_g_engine_stream+strm_clear_flg, ax
        xchg ah, al
        mov word ptr DGROUP:_g_engine_stream+strm_free_ent, ax
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jge lzw_code
lzw_end:
        pop di
        pop si
        pop bp
        ret
lzw_code:
        sub di, di
        mov si, ax
        mov word ptr DGROUP:_g_engine_stream+strm_incode, ax
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        jl lzw_unwind
        mov ax, word ptr DGROUP:_g_engine_stream+strm_finchar
        stosb
        mov si, word ptr DGROUP:_g_engine_stream+strm_oldcode
lzw_unwind:
        mov dx, ds
        mov ax, word ptr DGROUP:_g_engine_stream+strm_scratch+2
        mov ds, ax
        mov cx, 100h
        mov bx, 2720h
lzw_chain:
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        jmp short lzw_chain
lzw_chain_end:
        mov al, byte ptr [bx+si]
        stosb
        mov ds, dx
        mov ah, 0
        mov word ptr DGROUP:_g_engine_stream+strm_finchar, ax
        mov cx, word ptr DGROUP:_g_engine_stream+strm_wanted
        inc cx
        mov si, di
        dec si
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je lzw_skip
lzw_copy:
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
lzw_copy_next:
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        jmp short lzw_copy
lzw_skip:
        lodsb
        dec cx
        je lzw_wanted_met
        inc di
lzw_skip_next:
        sub si, bx
        jl lzw_string_done
        jmp short lzw_skip
lzw_wanted_met:
        mov ds, dx
        mov word ptr DGROUP:_g_engine_stream+strm_output, di
        mov word ptr DGROUP:lzw_resume_src, si
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov bl, byte ptr [si+res_spill_end]
        inc word ptr [si+res_spill_end]
        sub bh, bh
        mov si, word ptr DGROUP:_g_engine_stream+strm_spill
        mov byte ptr [bx+si], al
        sub ax, ax
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, ax
        inc ax
        mov byte ptr DGROUP:_g_engine_stream+strm_resume, al
        pop di
        pop si
        pop bp
        ret
lzw_string_done:
        mov ax, ds
        mov es, ax
        mov ds, dx
        dec cx
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, cx
        mov word ptr DGROUP:_g_engine_stream+strm_output, di
        mov ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        cmp ax, 1000h
        jge lzw_table_full
        mov di, ax
        shl di, 1
        mov ax, word ptr DGROUP:_g_engine_stream+strm_oldcode
        mov bx, es
        mov bp, es
        sub bx, 372h
        mov es, bx
        stosw
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_engine_stream+strm_free_ent, ax
        add di, 271fh
        mov ax, word ptr DGROUP:_g_engine_stream+strm_finchar
        stosb
        mov es, bp
lzw_table_full:
        mov ax, word ptr DGROUP:_g_engine_stream+strm_incode
        mov word ptr DGROUP:_g_engine_stream+strm_oldcode, ax
        jmp lzw_next_code
_decompress_lzw endp

/* 0x1cc65 */
_next_lzw_code proc near
        mov ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_maxcode
        jg code_widen
        cmp word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
        jne code_clear
        mov ax, word ptr DGROUP:_g_engine_stream+strm_bit_pos
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_bit_end
        jge code_refill
code_extract:
        mov si, offset DGROUP:lzw_code_buf
        mov bx, word ptr DGROUP:_g_engine_stream+strm_n_bits
        mov ch, al
        mov dx, ax
        add ax, bx
        mov word ptr DGROUP:_g_engine_stream+strm_bit_pos, ax
        shr dx, 1
        shr dx, 1
        shr dx, 1
        add si, dx
        and ch, 7
        lodsb
        mov cl, ch
        shr ax, cl
        mov dx, ax
        sub ch, 8
        neg ch
        sub bl, ch
        cmp bl, 8
        jl code_last_bits
        lodsb
        mov cl, ch
        shl ax, cl
        or dx, ax
        add ch, 8
        sub bl, 8
code_last_bits:
        sub ax, ax
        mov al, byte ptr lzw_rmask[bx]
        and al, byte ptr [si]
        mov cl, ch
        shl ax, cl
        or ax, dx
        ret
code_widen:
        mov cx, word ptr DGROUP:_g_engine_stream+strm_n_bits
        inc cx
        mov word ptr DGROUP:_g_engine_stream+strm_n_bits, cx
        mov ax, 1000h
        cmp cl, 0ch
        je code_set_max
        mov ax, 1
        shl ax, cl
        dec ax
code_set_max:
        mov word ptr DGROUP:_g_engine_stream+strm_maxcode, ax
        cmp word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
        je code_refill
code_clear:
        mov ax, 9
        mov word ptr DGROUP:_g_engine_stream+strm_n_bits, ax
        mov ax, 1ffh
        mov word ptr DGROUP:_g_engine_stream+strm_maxcode, ax
        mov word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
code_refill:
        mov si, word ptr DGROUP:_g_engine_stream+strm_n_bits
        push si
        mov ax, 35bch
        push ax
        call _read_input_block
        add sp, 4
        sub bx, bx
        cmp ax, bx
        jle code_eof
        mov word ptr DGROUP:_g_engine_stream+strm_bit_pos, bx
        shl ax, 1
        shl ax, 1
        shl ax, 1
        dec si
        sub ax, si
        mov word ptr DGROUP:_g_engine_stream+strm_bit_end, ax
        mov ax, bx
        jmp code_extract
code_eof:
        mov word ptr DGROUP:_g_engine_stream+strm_bit_end, ax
        mov ax, 0ffffh
        ret
_next_lzw_code endp

/* 0x1cd2c */
_rle_from_memory proc near
        push bp
        push si
        push di
        sub cx, cx
        mov bx, cx
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov cl, byte ptr [si+res_spill_end]
        mov ax, word ptr DGROUP:_g_engine_stream+strm_spill
        add ax, cx
        mov word ptr DGROUP:rle_spill_at, ax
        mov ax, word ptr [si+res_in_end]
        mov dx, word ptr [si+res_in_end+2]
        sub ax, word ptr [si+res_in_pos]
        sbb dx, word ptr [si+res_in_pos+2]
        mov bp, 0ffffh
        jne rle_start
        mov bp, ax
rle_start:
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov dx, word ptr DGROUP:_g_engine_stream+strm_wanted
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        lds si, dword ptr DGROUP:_g_engine_stream+strm_input
        test al, 40h
        je rle_skip
rle_copy:
        sub cx, cx
        cmp bx, bp
        je rle_input_done
        lodsb
        shl al, 1
        jae rle_literal
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jb rle_fill_spill
        mov ah, al
        test di, 1
        je rle_fill_words
        stosb
        dec cx
rle_fill_words:
        shr cx, 1
        rep stosw
        rcl cx, 1
        rep stosb
        jmp short rle_copy
rle_literal:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        sub dx, cx
        jb rle_literal_spill
        test di, 1
        je rle_literal_words
        movsb
        dec cx
rle_literal_words:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
        jmp short rle_copy
rle_input_done:
        mov bp, cx
        jmp short rle_return
rle_pad_90 db 90h
rle_skip:
        sub cx, cx
        cmp bx, bp
        je rle_input_done
        lodsb
        shl al, 1
        jae rle_skip_literal
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jae rle_skip
rle_fill_spill:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep stosb
rle_return:
        mov ax, ss
        mov ds, ax
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, bp
        add byte ptr [si+res_spill_end], al
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, dx
        add word ptr [si+res_in_pos], bx
        adc word ptr [si+res_in_pos+2], 0
        pop di
        pop si
        pop bp
        ret
rle_skip_literal:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        add si, cx
        sub dx, cx
        jae rle_skip
rle_literal_spill:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep movsb
        jmp short rle_return
_rle_from_memory endp
LZW_TEXT ends
}
#else

/*
 * **Nine bit masks**, DGROUP 0x35b2..0x35bc, `(1 << n) - 1` for n from 0 to 8
 * - the same nine as `g_engine_lzw_masks` - and a zero byte. **Not established**
 * which routine reads these; the port's decoders read the other copy.
 */
struct engine_bit_masks {
    uint8_t   mask[9];            /* +0x00 */
    uint8_t   pad_35bb;           /* +0x09 */
} PACKED;

struct engine_bit_masks g_engine_bit_masks = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };


/*
 * **The bit reader's input window**, DGROUP 0x35bc..0x35c8, 0x0c bytes: `next_lzw_code` has
 * `read_input_block` fill it and takes its codes out of it a byte at a time,
 * from the bit position g_engine_stream keeps. Twelve bytes, up to the mask table.
 */
struct engine_lzw_window {
    uint8_t   window[12];         /* +0x00 [0xc] */
#ifndef __TURBOC__
    /* OURS: a code that ends on the window's last byte reads one more and
       masks it with `mask[0]`, which is 0 - in DGROUP that byte is the mask
       table's first. The host's object has to have it. */
    uint8_t   over;
#endif
} PACKED;

struct engine_lzw_window g_engine_lzw_window = {
    .window = {
        0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
        0x0c,
    },
};

/*
 * **The LZW mask table**, DGROUP 0x35c8..0x35d1, 0x09 bytes: 0, 1, 3, 7, ..., 0xff, indexed
 * by how many bits are still wanted. Nine bytes, up to 0x35d1.
 */
struct engine_lzw_masks {
    uint8_t   mask[9];            /* +0x00 [9] */
} PACKED;

struct engine_lzw_masks g_engine_lzw_masks = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };

/*
 * **Where the LZW string had got to**, DGROUP 0x35d1..0x35d3, 0x02 bytes.
 *
 * `decompress_lzw` copies a decoded string out of its scratch buffer
 * backwards, and a request that fills mid-string has to resume there next
 * time. This is that position, as an offset into the scratch buffer, saved
 * beside the byte at DGROUP 0x58a2 that says a resume is pending.
 */
struct engine_lzw_resume {
    int16_t   scratch_at;         /* +0x00 [2] */
} PACKED;

struct engine_lzw_resume g_engine_lzw_resume;

/*
 * **Three bytes before the LZW coder's data**, DGROUP 0x35d3..0x35d6: the
 * assembly at 0x1cd2c keeps a near pointer at 0x35d4. The rest of the
 * writer's state is resfile.c's.
 */
struct engine_bit_state {
    uint8_t   pad_35d3[3];        /* +0x00 */
} PACKED;

struct engine_bit_state g_engine_bit_state;

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
     * **The scratch block, and the three things inside it.** The dictionary is
     * a word per code at +0 and a byte per code at +0x2720; the buffer the
     * string is built into, forwards, and then read out of backwards, is at
     * +0x3720 - which the original reaches as the segment plus 0x372
     * paragraphs, because the block's offset is zero.
     *
     * **It is zero by construction, not by luck.** `select_resource` files
     * either a DOS block, which starts a segment, or `g_scratch_block`, which
     * is masked to a paragraph with `& 0xfff0` and then normalised where it is
     * built. Every use here reads that invariant, and the clear loop below
     * writes the offset itself into the dictionary, which is what makes it
     * visible.
     */
    uint8_t far * block = g_engine_stream.scratch;
    uint16_t *prefix = (uint16_t *)(void *)block;
    uint8_t far * suffix = block + 0x2720;
    uint8_t far * scratch = block + 0x3720;
    uint8_t far *in, *back;
    /* The output cursor: `di` against the segment the caller chose, walked
       with `inc di` and filed back into DGROUP 0x5894 - a far pointer, whose
       arithmetic is the offset's. The scratch index that shared the `di`
       register is `in` above, which is a different thing entirely. */
    uint8_t far * out;
    uint16_t si, cx;
    int16_t code;
    uint8_t al = 0;
    int16_t copying;

    if (g_engine_stream.resume != 0) {
        cx = (uint16_t)(g_engine_stream.wanted + 1);
        out = (uint8_t far *)g_engine_stream.output;
        back = scratch + (uint16_t)g_engine_lzw_resume.scratch_at;
        copying = (g_engine_resource_flags.flags & 0x40) != 0;
        g_engine_stream.resume = 0;
        goto step_back;
    }

    if (g_engine_stream.first_code != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        g_engine_stream.first_code = 0;
        code = next_lzw_code();
        g_engine_stream.oldcode = code;
        g_engine_stream.finchar = code;
        emit_byte((uint16_t)code);
    }

    for (;;) {
        code = next_lzw_code();
        if (code < 0)
            return code;

        if (code == 0x100) {
            /* The block's own offset, written into every entry it clears -
               zero, as the note on `block` says, and written as the field
               rather than as a 0 because that is what the original stores. */
            uint16_t p = FP_OFF(g_engine_stream.scratch);
            int16_t i;

            for (i = 0; i < 0x100; i++)
                prefix[i] = p;

            g_engine_stream.clear_flg = (int16_t)(p + 1);
            g_engine_stream.free_ent = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        in = scratch;
        si = (uint16_t)code;
        g_engine_stream.incode = code;

        if ((int16_t)si >= g_engine_stream.free_ent) {
            *in++ = (uint8_t)((uint16_t)g_engine_stream.finchar);
            si = ((uint16_t)g_engine_stream.oldcode);
        }

        while (si >= 0x100) {
            *in++ = suffix[si];
            si = prefix[si];
        }

        al = suffix[si];
        *in++ = al;
        g_engine_stream.finchar = al;

        cx = (uint16_t)(g_engine_stream.wanted + 1);
        back = in - 1;
        out = (uint8_t far *)g_engine_stream.output;
        copying = (g_engine_resource_flags.flags & 0x40) != 0;

        for (;;) {
            al = *back++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                struct resource *rec;

                g_engine_stream.output = out;
                g_engine_lzw_resume.scratch_at = (int16_t)(back - scratch);

                rec = g_engine_stream.rec;
                {
                    uint16_t n = rec->spill_end;

                    /* 0x1cc0a is `inc word ptr [si+0x1a]`: a carry out of the
                       end lands in the start. */
                    if (++rec->spill_end == 0)
                        rec->spill_start++;
                    g_engine_stream.spill[n] = al;
                }

                g_engine_stream.wanted = 0;
                g_engine_stream.resume = 1;
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
        g_engine_stream.wanted = (int16_t)cx;
        g_engine_stream.output = out;

        if (g_engine_stream.free_ent < 0x1000) {
            uint16_t next = ((uint16_t)g_engine_stream.free_ent);

            prefix[next] = ((uint16_t)g_engine_stream.oldcode);
            g_engine_stream.free_ent = (int16_t)(next + 1);
            suffix[next] = (uint8_t)((uint16_t)g_engine_stream.finchar);
        }

        g_engine_stream.oldcode = g_engine_stream.incode;
    }
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

    if ((int16_t)((uint16_t)g_engine_stream.free_ent) > g_engine_stream.maxcode) {
        uint16_t cx = (uint16_t)(((uint16_t)g_engine_stream.n_bits) + 1);

        g_engine_stream.n_bits = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            g_engine_stream.maxcode = 0x1000;
        else
            g_engine_stream.maxcode = (int16_t)((1 << (cx & 0xff)) - 1);

        if (g_engine_stream.clear_flg != 0) {
            g_engine_stream.n_bits = 9;
            g_engine_stream.maxcode = 0x1ff;
            g_engine_stream.clear_flg = 0;
        }
    } else if (g_engine_stream.clear_flg != 0) {
        g_engine_stream.n_bits = 9;
        g_engine_stream.maxcode = 0x1ff;
        g_engine_stream.clear_flg = 0;
    } else if (g_engine_stream.bit_pos < g_engine_stream.bit_end) {
        goto extract;
    }

    {
        uint16_t width = ((uint16_t)g_engine_stream.n_bits);
        int16_t n = read_input_block(g_engine_lzw_window.window, width);

        if (n <= 0) {
            g_engine_stream.bit_end = n;
            return -1;
        }

        g_engine_stream.bit_pos = 0;
        g_engine_stream.bit_end = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = ((uint16_t)g_engine_stream.bit_pos);
    bl = (uint8_t)((uint16_t)g_engine_stream.n_bits);
    ch = (uint8_t)bitpos;

    g_engine_stream.bit_pos = (int16_t)(bitpos + ((uint16_t)g_engine_stream.n_bits));

    in = &g_engine_lzw_window.window[bitpos >> 3];
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

    ax = g_engine_lzw_masks.mask[bl];
    ax &= *in;
    ax = (uint16_t)(ax << ch);

    return (int16_t)(ax | dx);
}

/*
 * 0x1cd2c
 *
 * Decompression type 1 from a resource in memory, hand-written assembly. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near rle_from_memory(void)
{
    not_transcribed("0x1cd2c, rle_from_memory");
    return 0;
}
#endif
