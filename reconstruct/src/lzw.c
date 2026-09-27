/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
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
 * JUDGE: assembler tasm1.01
 */
#include <string.h>

#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
_DATA segment word public 'DATA'
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh, 0h
d_35bc label byte
        db 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch
d_35c8 label byte
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh
d_35d1 label byte
        db 0h, 0h, 0h
d_35d4 label byte
        db 0h, 0h
_DATA ends

extrn _ENGINE_RESOURCE_FLAGS:byte
extrn _ENGINE_STREAM:byte
LZW_TEXT segment byte public 'CODE'
assume cs:LZW_TEXT, ds:DGROUP
extrn _emit_byte:near
extrn _read_input_block:near
public _decompress_lzw, _next_lzw_code, _rle_from_memory
L1ca46:
        mov byte ptr DGROUP:_ENGINE_STREAM+26h, 0
        mov bp, es
        call _next_lzw_code
        mov word ptr DGROUP:_ENGINE_STREAM+1eh, ax
        mov word ptr DGROUP:_ENGINE_STREAM+24h, ax
        push ax
        call _emit_byte
        add sp, 2
        mov es, bp
        jmp short L1caa3
c_1ca61 db 90h

/* 0x1ca62 */
_decompress_lzw proc near
        push bp
        push si
        push di
        mov ax, word ptr DGROUP:_ENGINE_STREAM+6h
        add ax, 372h
        mov es, ax
        cmp byte ptr DGROUP:_ENGINE_STREAM+1ah, 0
        je L1ca9c
        mov cx, word ptr DGROUP:_ENGINE_STREAM+8h
        inc cx
        mov bp, es
        les di, dword ptr DGROUP:_ENGINE_STREAM+0ch
        mov al, byte ptr DGROUP:_ENGINE_RESOURCE_FLAGS
        mov si, word ptr DGROUP:d_35d1
        mov byte ptr DGROUP:_ENGINE_STREAM+1ah, 0
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je L1ca99
        jmp L1cba0
L1ca99:
        jmp L1cbf3
L1ca9c:
        cmp byte ptr DGROUP:_ENGINE_STREAM+26h, 0
        jne L1ca46
L1caa3:
        mov bp, es
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jl L1cad4
        cmp ax, 100h
        jne L1cad8
        mov bp, es
        les di, dword ptr DGROUP:_ENGINE_STREAM+4h
        mov ax, di
        mov cx, 100h
        rep stosw
        inc ax
        mov word ptr DGROUP:_ENGINE_STREAM+1ch, ax
        xchg ah, al
        mov word ptr DGROUP:_ENGINE_STREAM+18h, ax
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jge L1cad8
L1cad4:
        pop di
        pop si
        pop bp
        ret
L1cad8:
        sub di, di
        mov si, ax
        mov word ptr DGROUP:_ENGINE_STREAM+28h, ax
        cmp ax, word ptr DGROUP:_ENGINE_STREAM+18h
        jl L1caed
        mov ax, word ptr DGROUP:_ENGINE_STREAM+24h
        stosb
        mov si, word ptr DGROUP:_ENGINE_STREAM+1eh
L1caed:
        mov dx, ds
        mov ax, word ptr DGROUP:_ENGINE_STREAM+6h
        mov ds, ax
        mov cx, 100h
        mov bx, 2720h
L1cafa:
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl L1cb75
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        jmp short L1cafa
L1cb75:
        mov al, byte ptr [bx+si]
        stosb
        mov ds, dx
        mov ah, 0
        mov word ptr DGROUP:_ENGINE_STREAM+24h, ax
        mov cx, word ptr DGROUP:_ENGINE_STREAM+8h
        inc cx
        mov si, di
        dec si
        mov bp, es
        les di, dword ptr DGROUP:_ENGINE_STREAM+0ch
        mov al, byte ptr DGROUP:_ENGINE_RESOURCE_FLAGS
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je L1cbee
L1cb9b:
        lodsb
        dec cx
        je L1cbf9
        stosb
L1cba0:
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        lodsb
        dec cx
        je L1cbf9
        stosb
        sub si, bx
        jl L1cc22
        jmp short L1cb9b
L1cbee:
        lodsb
        dec cx
        je L1cbf9
        inc di
L1cbf3:
        sub si, bx
        jl L1cc22
        jmp short L1cbee
L1cbf9:
        mov ds, dx
        mov word ptr DGROUP:_ENGINE_STREAM+0ch, di
        mov word ptr DGROUP:d_35d1, si
        mov si, word ptr DGROUP:_ENGINE_STREAM+2h
        mov bl, byte ptr [si+1ah]
        inc word ptr [si+1ah]
        sub bh, bh
        mov si, word ptr DGROUP:_ENGINE_STREAM+0ah
        mov byte ptr [bx+si], al
        sub ax, ax
        mov word ptr DGROUP:_ENGINE_STREAM+8h, ax
        inc ax
        mov byte ptr DGROUP:_ENGINE_STREAM+1ah, al
        pop di
        pop si
        pop bp
        ret
L1cc22:
        mov ax, ds
        mov es, ax
        mov ds, dx
        dec cx
        mov word ptr DGROUP:_ENGINE_STREAM+8h, cx
        mov word ptr DGROUP:_ENGINE_STREAM+0ch, di
        mov ax, word ptr DGROUP:_ENGINE_STREAM+18h
        cmp ax, 1000h
        jge L1cc5c
        mov di, ax
        shl di, 1
        mov ax, word ptr DGROUP:_ENGINE_STREAM+1eh
        mov bx, es
        mov bp, es
        sub bx, 372h
        mov es, bx
        stosw
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_ENGINE_STREAM+18h, ax
        add di, 271fh
        mov ax, word ptr DGROUP:_ENGINE_STREAM+24h
        stosb
        mov es, bp
L1cc5c:
        mov ax, word ptr DGROUP:_ENGINE_STREAM+28h
        mov word ptr DGROUP:_ENGINE_STREAM+1eh, ax
        jmp L1caa3
_decompress_lzw endp

/* 0x1cc65 */
_next_lzw_code proc near
        mov ax, word ptr DGROUP:_ENGINE_STREAM+18h
        cmp ax, word ptr DGROUP:_ENGINE_STREAM+2eh
        jg L1ccc8
        cmp word ptr DGROUP:_ENGINE_STREAM+1ch, 0
        jne L1cce9
        mov ax, word ptr DGROUP:_ENGINE_STREAM+2ah
        cmp ax, word ptr DGROUP:_ENGINE_STREAM+2ch
        jge L1ccfb
L1cc7e:
        mov si, offset DGROUP:d_35bc
        mov bx, word ptr DGROUP:_ENGINE_STREAM+16h
        mov ch, al
        mov dx, ax
        add ax, bx
        mov word ptr DGROUP:_ENGINE_STREAM+2ah, ax
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
        jl L1ccb9
        lodsb
        mov cl, ch
        shl ax, cl
        or dx, ax
        add ch, 8
        sub bl, 8
L1ccb9:
        sub ax, ax
        mov al, byte ptr d_35c8[bx]
        and al, byte ptr [si]
        mov cl, ch
        shl ax, cl
        or ax, dx
        ret
L1ccc8:
        mov cx, word ptr DGROUP:_ENGINE_STREAM+16h
        inc cx
        mov word ptr DGROUP:_ENGINE_STREAM+16h, cx
        mov ax, 1000h
        cmp cl, 0ch
        je L1ccdf
        mov ax, 1
        shl ax, cl
        dec ax
L1ccdf:
        mov word ptr DGROUP:_ENGINE_STREAM+2eh, ax
        cmp word ptr DGROUP:_ENGINE_STREAM+1ch, 0
        je L1ccfb
L1cce9:
        mov ax, 9
        mov word ptr DGROUP:_ENGINE_STREAM+16h, ax
        mov ax, 1ffh
        mov word ptr DGROUP:_ENGINE_STREAM+2eh, ax
        mov word ptr DGROUP:_ENGINE_STREAM+1ch, 0
L1ccfb:
        mov si, word ptr DGROUP:_ENGINE_STREAM+16h
        push si
        mov ax, 35bch
        push ax
        call _read_input_block
        add sp, 4
        sub bx, bx
        cmp ax, bx
        jle L1cd25
        mov word ptr DGROUP:_ENGINE_STREAM+2ah, bx
        shl ax, 1
        shl ax, 1
        shl ax, 1
        dec si
        sub ax, si
        mov word ptr DGROUP:_ENGINE_STREAM+2ch, ax
        mov ax, bx
        jmp L1cc7e
L1cd25:
        mov word ptr DGROUP:_ENGINE_STREAM+2ch, ax
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
        mov si, word ptr DGROUP:_ENGINE_STREAM+2h
        mov cl, byte ptr [si+1ah]
        mov ax, word ptr DGROUP:_ENGINE_STREAM+0ah
        add ax, cx
        mov word ptr DGROUP:d_35d4, ax
        mov ax, word ptr [si+0eh]
        mov dx, word ptr [si+10h]
        sub ax, word ptr [si+0ah]
        sbb dx, word ptr [si+0ch]
        mov bp, 0ffffh
        jne L1cd55
        mov bp, ax
L1cd55:
        les di, dword ptr DGROUP:_ENGINE_STREAM+0ch
        mov dx, word ptr DGROUP:_ENGINE_STREAM+8h
        mov al, byte ptr DGROUP:_ENGINE_RESOURCE_FLAGS
        lds si, dword ptr DGROUP:_ENGINE_STREAM+10h
        test al, 40h
        je L1cdb5
L1cd68:
        sub cx, cx
        cmp bx, bp
        je L1cdb0
        lodsb
        shl al, 1
        jae L1cd93
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jb L1cdcc
        mov ah, al
        test di, 1
        je L1cd89
        stosb
        dec cx
L1cd89:
        shr cx, 1
        rep stosw
        rcl cx, 1
        rep stosb
        jmp short L1cd68
L1cd93:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        sub dx, cx
        jb L1ce09
        test di, 1
        je L1cda6
        movsb
        dec cx
L1cda6:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
        jmp short L1cd68
L1cdb0:
        mov bp, cx
        jmp short L1cde0
c_1cdb4 db 90h
L1cdb5:
        sub cx, cx
        cmp bx, bp
        je L1cdb0
        lodsb
        shl al, 1
        jae L1cdfc
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jae L1cdb5
L1cdcc:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep stosb
L1cde0:
        mov ax, ss
        mov ds, ax
        mov si, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, bp
        add byte ptr [si+1ah], al
        mov word ptr DGROUP:_ENGINE_STREAM+8h, dx
        add word ptr [si+0ah], bx
        adc word ptr [si+0ch], 0
        pop di
        pop si
        pop bp
        ret
L1cdfc:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        add si, cx
        sub dx, cx
        jae L1cdb5
L1ce09:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep movsb
        jmp short L1cde0
_rle_from_memory endp
LZW_TEXT ends
}
#else

/*
 * **Nine bit masks**, DGROUP 0x35b2..0x35bc, `(1 << n) - 1` for n from 0 to 8
 * - the same nine as `ENGINE_LZW_MASKS` - and a zero byte. **Not established**
 * which routine reads these; the port's decoders read the other copy.
 */
struct engine_bit_masks {
    uint8_t   mask[9];            /* +0x00 */
    uint8_t   pad_35bb;           /* +0x09 */
} PACKED;

struct engine_bit_masks ENGINE_BIT_MASKS DGROUP_AT(0x35b2) = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };


/*
 * **The bit reader's input window**, DGROUP 0x35bc..0x35c8, 0x0c bytes: `next_lzw_code` has
 * `read_input_block` fill it and takes its codes out of it a byte at a time,
 * from the bit position ENGINE_STREAM keeps. Twelve bytes, up to the mask table.
 */
struct engine_lzw_window {
    uint8_t   window[12];         /* +0x00 [0xc] */
} PACKED;

struct engine_lzw_window ENGINE_LZW_WINDOW DGROUP_AT(0x35bc) = {
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

struct engine_lzw_masks ENGINE_LZW_MASKS DGROUP_AT(0x35c8) = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };

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

struct engine_lzw_resume ENGINE_LZW_RESUME DGROUP_AT(0x35d1);

/*
 * **Three bytes before the LZW coder's data**, DGROUP 0x35d3..0x35d6: the
 * assembly at 0x1cd2c keeps a near pointer at 0x35d4. The rest of the
 * writer's state is resfile.c's.
 */
struct engine_bit_state {
    uint8_t   pad_35d3[3];        /* +0x00 */
} PACKED;

struct engine_bit_state ENGINE_BIT_STATE DGROUP_AT(0x35d3);

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
     * either a DOS block, which starts a segment, or `DG3576.scratch`, which
     * is masked to a paragraph with `& 0xfff0` and then normalised where it is
     * built. Every use here reads that invariant, and the clear loop below
     * writes the offset itself into the dictionary, which is what makes it
     * visible.
     */
    uint8_t far * block = ENGINE_STREAM.scratch;
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

    if (ENGINE_STREAM.resume != 0) {
        cx = (uint16_t)(ENGINE_STREAM.wanted + 1);
        out = (uint8_t far *)ENGINE_STREAM.out;
        back = scratch + (uint16_t)ENGINE_LZW_RESUME.scratch_at;
        copying = (ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0;
        ENGINE_STREAM.resume = 0;
        goto step_back;
    }

    if (ENGINE_STREAM.first_code != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        ENGINE_STREAM.first_code = 0;
        code = next_lzw_code();
        ENGINE_STREAM.oldcode = code;
        ENGINE_STREAM.finchar = code;
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
            uint16_t p = FP_OFF(ENGINE_STREAM.scratch);
            int16_t i;

            for (i = 0; i < 0x100; i++)
                prefix[i] = p;

            ENGINE_STREAM.clear_flg = (int16_t)(p + 1);
            ENGINE_STREAM.free_ent = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        in = scratch;
        si = (uint16_t)code;
        ENGINE_STREAM.incode = code;

        if ((int16_t)si >= ENGINE_STREAM.free_ent) {
            *in++ = (uint8_t)((uint16_t)ENGINE_STREAM.finchar);
            si = ((uint16_t)ENGINE_STREAM.oldcode);
        }

        while (si >= 0x100) {
            *in++ = suffix[si];
            si = prefix[si];
        }

        al = suffix[si];
        *in++ = al;
        ENGINE_STREAM.finchar = al;

        cx = (uint16_t)(ENGINE_STREAM.wanted + 1);
        back = in - 1;
        out = (uint8_t far *)ENGINE_STREAM.out;
        copying = (ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0;

        for (;;) {
            al = *back++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                struct resource *rec;

                ENGINE_STREAM.out = out;
                ENGINE_LZW_RESUME.scratch_at = (int16_t)(back - scratch);

                rec = ENGINE_STREAM.rec;
                {
                    uint16_t n = rec->spill_end;

                    /* 0x1cc0a is `inc word ptr [si+0x1a]`: a carry out of the
                       end lands in the start. */
                    if (++rec->spill_end == 0)
                        rec->spill_start++;
                    ENGINE_STREAM.spill[n] = al;
                }

                ENGINE_STREAM.wanted = 0;
                ENGINE_STREAM.resume = 1;
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
        ENGINE_STREAM.wanted = (int16_t)cx;
        ENGINE_STREAM.out = out;

        if (ENGINE_STREAM.free_ent < 0x1000) {
            uint16_t next = ((uint16_t)ENGINE_STREAM.free_ent);

            prefix[next] = ((uint16_t)ENGINE_STREAM.oldcode);
            ENGINE_STREAM.free_ent = (int16_t)(next + 1);
            suffix[next] = (uint8_t)((uint16_t)ENGINE_STREAM.finchar);
        }

        ENGINE_STREAM.oldcode = ENGINE_STREAM.incode;
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

    if ((int16_t)((uint16_t)ENGINE_STREAM.free_ent) > ENGINE_STREAM.maxcode) {
        uint16_t cx = (uint16_t)(((uint16_t)ENGINE_STREAM.n_bits) + 1);

        ENGINE_STREAM.n_bits = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            ENGINE_STREAM.maxcode = 0x1000;
        else
            ENGINE_STREAM.maxcode = (int16_t)((1 << (cx & 0xff)) - 1);

        if (ENGINE_STREAM.clear_flg != 0) {
            ENGINE_STREAM.n_bits = 9;
            ENGINE_STREAM.maxcode = 0x1ff;
            ENGINE_STREAM.clear_flg = 0;
        }
    } else if (ENGINE_STREAM.clear_flg != 0) {
        ENGINE_STREAM.n_bits = 9;
        ENGINE_STREAM.maxcode = 0x1ff;
        ENGINE_STREAM.clear_flg = 0;
    } else if (ENGINE_STREAM.bit_pos < ENGINE_STREAM.bit_end) {
        goto extract;
    }

    {
        uint16_t width = ((uint16_t)ENGINE_STREAM.n_bits);
        int16_t n = read_input_block(ENGINE_LZW_WINDOW.window, width);

        if (n <= 0) {
            ENGINE_STREAM.bit_end = n;
            return -1;
        }

        ENGINE_STREAM.bit_pos = 0;
        ENGINE_STREAM.bit_end = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = ((uint16_t)ENGINE_STREAM.bit_pos);
    bl = (uint8_t)((uint16_t)ENGINE_STREAM.n_bits);
    ch = (uint8_t)bitpos;

    ENGINE_STREAM.bit_pos = (int16_t)(bitpos + ((uint16_t)ENGINE_STREAM.n_bits));

    in = &ENGINE_LZW_WINDOW.window[bitpos >> 3];
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

    ax = ENGINE_LZW_MASKS.mask[bl];
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
