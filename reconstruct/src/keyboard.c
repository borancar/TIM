/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The keyboard driver**: installing and removing its interrupt handler,
 * the handler itself, and the two ways the game asks it for keys - plus the
 * video driver's `copy_rect` and `show_page` thunks at either end.
 *
 * A module of the original's **code segment 1c25**, image 0x21088..0x2149e,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x458c..0x471c,
 * the handler's state and tables, and it keeps four words in its own code
 * segment at 0x2108c - the saved keyboard and timer vectors - which only
 * these routines name.
 *
 * **So it is TASM source**: the handler is an interrupt routine and every
 * entry is hand-written - it even patches its own immediates, `cs:` stores
 * into two `and dl, 0ffh`. The source is the `#ifdef __TURBOC__` block
 * below, the host's transcription the `#else`.
 *
 * **Where the module begins is proven to within one thunk**:
 * `install_keyboard` calls `detect_pcjr` (0x20be0) through TLINK's
 * `nop / push cs / call`, so that routine is in another module, and the
 * code-segment words at 0x2108c are this module's. Whether the thunk at
 * 0x21088 before them is its first routine or the previous module's last is
 * not settled. Its end is the text module's first byte.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler tasm1.01
 */
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
d_458c label byte
        db 0h
d_458d label byte
        db 0h
d_458e label byte
        db 0h, 0h
d_4590 label byte
        db 0h, 0h, 1h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
d_45da label byte
        db 0h, 1bh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h, 2dh, 3dh, 8h, 9h
        db 71h, 77h, 65h, 72h, 74h, 79h, 75h, 69h, 6fh, 70h, 5bh, 5dh, 0dh, 84h, 61h, 73h
        db 64h, 66h, 67h, 68h, 6ah, 6bh, 6ch, 3bh, 27h, 60h, 82h, 5ch, 7ah, 78h, 63h, 76h
        db 62h, 6eh, 6dh, 2ch, 2eh, 2fh, 81h, 2ah, 88h, 20h, 0c0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
d_4633 label byte
        db 0h, 1bh, 21h, 40h, 23h, 24h, 25h, 5eh, 26h, 2ah, 28h, 29h, 5fh, 2bh, 8h, 0h
        db 51h, 57h, 45h, 52h, 54h, 59h, 55h, 49h, 4fh, 50h, 7bh, 7dh, 0dh, 84h, 41h, 53h
        db 44h, 46h, 47h, 48h, 4ah, 4bh, 4ch, 3ah, 22h, 7eh, 82h, 7ch, 5ah, 58h, 43h, 56h
        db 42h, 4eh, 4dh, 3ch, 3eh, 3fh, 81h, 0h, 88h, 20h, 0c0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 37h, 38h, 39h, 2dh, 34h, 35h, 36h, 2bh, 31h
        db 32h, 33h, 30h, 2eh, 0h, 0h, 0h, 0h, 0h
d_468c label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 6eh, 7eh, 8eh, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 2h, 2h, 0h, 0h, 5eh, 9eh
        db 1eh, 6h, 6h, 6h, 6h, 6h, 6h, 0h, 0h, 0h, 0h, 2h, 4eh, 3eh, 2eh, 6h
        db 6h, 6h, 6h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 60h, 70h, 80h, 0h, 50h, 90h, 10h, 0h, 40h
        db 30h, 20h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
d_46e5 label byte
        db 0h, 0h, 0h, 0h
d_46e9 label byte
        db 0h, 0h, 0h, 0h
d_46ed label byte
        db 1h, 0h, 1h, 0h, 3h, 0h
d_46f3 label byte
        db 80h, 0a0h, 40h, 0e0h, 0c0h, 0f0h, 60h, 0b0h, 0h, 8h, 0ah, 2h, 6h, 4h, 5h, 1h
        db 9h, 0h
d_4705 label byte
        db 59h, 5ah, 55h, 54h, 29h, 58h, 2bh, 4ah, 4eh, 56h, 57h, 57h, 58h, 52h, 46h, 48h
        db 47h, 4bh, 50h, 4dh, 53h, 1ch
d_471b label byte
        db 0h
_DATA ends

extrn _detect_pcjr:far
extrn _game_teardown:far
extrn _DG4342:byte
extrn _VMDS:byte
KEYBOARD_TEXT segment byte public 'CODE'
assume cs:KEYBOARD_TEXT, ds:DGROUP
public _copy_rect_thunk, _install_keyboard, _remove_keyboard, _keyboard_isr
public _bios_read_key, _key_is_down, _show_page_thunk

/* 0x21088 */
_copy_rect_thunk proc near
        jmp dword ptr DGROUP:_DG4342+14h
c_2108c db 0h, 0h
c_2108e db 0h, 0h
c_21090 db 0h, 0h
c_21092 db 0h, 0h
_copy_rect_thunk endp

/* 0x21094 */
_install_keyboard proc far
        push bp
        mov bp, sp
        push di
        push si
        sub ax, ax
        cmp byte ptr DGROUP:d_458c, al
        je L210a4
        jmp L21139
L210a4:
        push ds
        mov ax, 3509h
        int 21h
        mov word ptr cs:c_2108c, bx
        mov word ptr cs:c_2108e, es
        mov ax, 351ch
        int 21h
        mov word ptr cs:c_21090, bx
        mov word ptr cs:c_21092, es
        mov dx, 4f46h
        mov ax, 1c25h
        mov ds, ax
        mov ax, 2509h
        int 21h
        mov ax, word ptr [bp+6]
        neg ax
        jae L210e4
        mov dx, 5136h
        mov ax, 1c25h
        mov ds, ax
        mov ax, 251ch
        int 21h
L210e4:
        pop ds
        mov byte ptr DGROUP:d_471b, 0
        call FAR PTR _detect_pcjr
        neg ax
        jae L21133
        int 15h
        jae L2110c
        mov ax, 40h
        mov es, ax
        mov si, 96h
        cmp byte ptr es:[si], 10h
        jne L2110c
        mov byte ptr DGROUP:d_471b, 1
        jmp short L21133
L2110c:
        mov byte ptr cs:c_21222, 0feh
        mov byte ptr cs:c_2122e, 0feh
        mov bx, offset DGROUP:d_468c
        mov al, byte ptr [bx+48h]
        mov byte ptr [bx+29h], al
        mov al, byte ptr [bx+4bh]
        mov byte ptr [bx+2bh], al
        mov al, byte ptr [bx+4dh]
        mov byte ptr [bx+4eh], al
        mov al, byte ptr [bx+50h]
        mov byte ptr [bx+4ah], al
L21133:
        mov ax, 1
        mov byte ptr DGROUP:d_458c, al
L21139:
        mov ax, 40h
        mov es, ax
        and byte ptr es:[17h], 0dfh
        test byte ptr DGROUP:d_458d, 0ffh
        je L21151
        or byte ptr es:[17h], 40h
L21151:
        mov al, byte ptr DGROUP:d_458c
        pop si
        pop di
        pop bp
        retf
_install_keyboard endp

/* 0x21158 */
_remove_keyboard proc far
        sub ax, ax
        cmp byte ptr DGROUP:d_458c, al
        je L21195
        mov byte ptr DGROUP:d_458c, al
        mov ax, 40h
        mov es, ax
        mov ax, word ptr es:[1ch]
        mov word ptr es:[1ah], ax
        push ds
        mov dx, word ptr cs:c_2108c
        mov ax, word ptr cs:c_2108e
        mov ds, ax
        mov ax, 2509h
        int 21h
        mov dx, word ptr cs:c_21090
        mov ax, word ptr cs:c_21092
        mov ds, ax
        mov ax, 251ch
        int 21h
        pop ds
        mov ax, 1
L21195:
        retf
_remove_keyboard endp

/* 0x21196 */
_keyboard_isr proc near
        push ax
        push bx
        push cx
        push dx
        push ds
        push es
        push di
        mov ax, 40h
        mov es, ax
        mov ax, DGROUP
        mov ds, ax
        xor ax, ax
        in al, 60h
        mov dx, 61h
        mov bx, ax
        in al, dx
        mov ah, al
        or al, 80h
        out dx, al
        mov al, ah
        out dx, al
        mov ax, bx
        and al, 7fh
        and bl, 80h
        cmp byte ptr DGROUP:_VMDS+1ch, 1
        jne L211ef
        cmp byte ptr DGROUP:d_471b, 1
        je L211e3
        mov di, offset DGROUP:d_4705
        dec di
        mov cx, 0bh
L211d5:
        inc di
        cmp al, byte ptr [di]
        loopne L211d5
        jne L211ef
        add di, 0bh
        mov al, byte ptr [di]
        jmp short L211ef
L211e3:
        cmp al, 29h
        jne L211e9
        mov al, 48h
L211e9:
        cmp al, 2bh
        jne L211ef
        mov al, 4bh
L211ef:
        or al, bl
        mov dh, 0ffh
        mov dl, al
        shl dx, 1
        shr dl, 1
        cmp dl, 59h
        jl L21201
        jmp L2137a
L21201:
        xor bh, bh
        mov bl, dl
        mov dl, byte ptr d_468c[bx]
        and dh, dl
        xor dh, 1
        mov byte ptr d_468c[bx], dh
        cmp byte ptr DGROUP:d_471b, 1
        jne L2122f
        cmp bl, 3ah
        jne L21225
        mov al, bl
        and dl, 0ffh
c_21222 equ byte ptr $-1
        jmp short L2122f
L21225:
        cmp bl, 45h
        jne L2122f
        mov al, bl
        and dl, 0ffh
c_2122e equ byte ptr $-1
L2122f:
        mov bx, offset DGROUP:d_45da
        test al, 80h
        je L21274
        test dl, 0f8h
        je L21259
        mov cx, dx
        mov ch, 0
        shr cx, 1
        shr cx, 1
        shr cx, 1
        mov di, cx
        shr cx, 1
        and di, 1
        mov ch, byte ptr d_4590[di]
        cmp ch, cl
        jne L21259
        mov byte ptr d_4590[di], 0
L21259:
        mov word ptr DGROUP:d_458e, 0
        and al, 7fh
        xlatb
        test al, 80h
        je L21271
        test al, 70h
        jne L21271
        xor al, 7fh
        and byte ptr es:[17h], al
L21271:
        jmp L2137a
L21274:
        test dl, 0f8h
        je L2128e
        mov cx, dx
        mov ch, 0
        shr cx, 1
        shr cx, 1
        shr cx, 1
        mov di, cx
        shr cx, 1
        and di, 1
        mov byte ptr d_4590[di], cl
L2128e:
        mov cl, al
        xor ah, ah
        mov di, ax
        xlatb
        test al, 80h
        je L212be
        and al, 7fh
        test al, 70h
        jne L212a7
        or byte ptr es:[17h], al
        jmp L2137a
L212a7:
        test al, 40h
        je L212b2
        test byte ptr DGROUP:d_458d, 0ffh
        jne L212bb
L212b2:
        shr dl, 1
        jb L212bb
        xor byte ptr es:[17h], al
L212bb:
        jmp L2137a
L212be:
        test byte ptr es:[17h], 4
        je L212d1
        or al, 80h
        test dl, 4
        je L212ee
        sub al, 20h
        jmp short L212ee
L212d1:
        test byte ptr es:[17h], 40h
        je L212e2
        test dl, 4
        je L212e2
        sub al, 20h
        jmp short L212ee
L212e2:
        test byte ptr es:[17h], 3
        je L212ee
        mov al, byte ptr d_4633[di]
L212ee:
        mov ah, cl
        mov word ptr DGROUP:d_458e, ax
        mov cx, word ptr es:[1ah]
        mov di, word ptr es:[1ch]
        cmp cx, 3ch
        je L2130a
        inc cx
        inc cx
        cmp cx, di
        je L21322
        jmp short L2130f
L2130a:
        cmp di, 1eh
        je L21322
L2130f:
        mov word ptr es:[di], ax
        cmp di, 3ch
        jne L2131a
        mov di, 1ch
L2131a:
        add di, 2
        mov word ptr es:[1ch], di
L21322:
        cmp ah, 20h
        jne L21335
        test byte ptr es:[17h], 4
        je L21335
        mov al, 20h
        out 20h, al
        jmp short L2137e
L21335:
        xor bx, bx
        cmp ax, 19bh
        je L2134a
        inc bx
        cmp ax, 5380h
        jne L2137a
        test byte ptr es:[17h], 8
        je L2137a
L2134a:
        test byte ptr es:[17h], 4
        je L2137a
        sub di, 2
        cmp di, 1ch
        jne L2135d
        mov di, 3ch
L2135d:
        mov word ptr es:[di], 0
        mov ax, word ptr es:[1ch]
        mov word ptr es:[1ah], ax
        mov al, 20h
        out 20h, al
        sti
        mov ax, bx
        push ax
        call FAR PTR _game_teardown
        pop ax
        jmp short L2137e
L2137a:
        mov al, 20h
        out 20h, al
L2137e:
        pop di
        pop es
        pop ds
        pop dx
        pop cx
        pop bx
        pop ax
        iret
c_21386 db 50h
c_21387 db 53h
c_21388 db 51h
c_21389 db 52h
c_2138a db 57h
c_2138b db 56h
c_2138c db 55h
c_2138d db 1eh
c_2138e db 6h
c_2138f db 0b8h, 3ch, 2dh
c_21392 db 8eh, 0d8h
c_21394 db 0a1h, 0f1h, 46h
c_21397 db 8eh, 0c0h
c_21399 db 0bdh, 7fh, 0h
c_2139c db 0beh, 2h, 0h
c_2139f db 0b7h, 0h
L213a1:
        sub ax, ax
        shr si, 1
        mov bl, byte ptr d_4590[si]
        shl si, 1
        dec bl
        jns L213bf
        cmp word ptr d_46ed[si], ax
        je L21422
L213b5:
        mov word ptr d_46e5[si], ax
        mov word ptr d_46e9[si], ax
        jmp short L21422
L213bf:
        cmp bl, 8
        je L213b5
        mov bl, byte ptr d_46f3[bx]
        shl bl, 1
        jae L213f6
        mov ax, word ptr d_46e5[si]
        cwd
        xor ax, dx
        sub ax, dx
        mov cx, ax
        mov di, dx
        mov ah, bl
        shl bl, 1
        cwd
        xor dx, di
        mov ax, es
        xor ax, dx
        sub ax, dx
        add ax, cx
        cmp ax, bp
        jle L213ee
        mov ax, bp
L213ee:
        xor ax, di
        sub ax, di
        mov word ptr d_46e5[si], ax
L213f6:
        shl bl, 1
        jae L21422
        mov ax, word ptr d_46e9[si]
        cwd
        xor ax, dx
        sub ax, dx
        mov cx, ax
        mov di, dx
        mov ah, bl
        cwd
        xor dx, di
        mov ax, es
        xor ax, dx
        sub ax, dx
        add ax, cx
        cmp ax, bp
        jle L2141a
        mov ax, bp
L2141a:
        xor ax, di
        sub ax, di
        mov word ptr d_46e9[si], ax
L21422:
        sub si, 2
        js L2142a
        jmp L213a1
L2142a:
        pop es
        pop ds
        pop bp
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop ax
        iret
_keyboard_isr endp

/* 0x21434 */
_bios_read_key proc far
        pushf
        cli
        push es
        mov ax, 40h
        mov es, ax
        xor ax, ax
        mov bx, word ptr es:[1ah]
        cmp bx, word ptr es:[1ch]
        je L21460
        mov ax, word ptr es:[bx]
        inc bx
        inc bx
        cmp bx, word ptr es:[82h]
        jne L2145b
        mov bx, word ptr es:[80h]
L2145b:
        mov word ptr es:[1ah], bx
L21460:
        pop es
        popf
        retf
c_21463 db 8bh, 0d5h
c_21465 db 8bh, 0ech
c_21467 db 2bh, 0dbh
c_21469 db 8bh, 46h, 4h
c_2146c db 0f7h, 0d8h
c_2146e db 0d1h, 0d3h
c_21470 db 8ah, 9fh, 90h, 45h
c_21474 db 8ah, 0e7h
c_21476 db 8ah, 87h, 0fbh, 46h
c_2147a db 8bh, 0eah
c_2147c db 0cbh
_bios_read_key endp

/* 0x2147d */
_key_is_down proc far
        cli
        mov dx, bp
        mov bp, sp
        mov bx, word ptr [bp+4]
        xor ax, ax
        mov al, byte ptr d_468c[bx]
        and al, 1
        mov bp, dx
        sti
        retf
c_21491 db 0cbh
c_21492 db 0ffh, 2eh, 0aah, 43h
c_21496 db 0ffh, 2eh, 0aeh, 43h
_key_is_down endp

/* 0x2149a */
_show_page_thunk proc near
        jmp dword ptr DGROUP:_DG4342+24h
_show_page_thunk endp
KEYBOARD_TEXT ends
}
#else

/*
 * **The keyboard handler's own state and tables**, DGROUP 0x458c..0x471b,
 * 0x18f bytes: whether it is installed, the last event it made, which keys are
 * held, and the tables the ISR translates through.
 */
struct engine_keyboard {
    uint8_t   installed;          /* +0x00 [1]  the keyboard handler is in: install_keyboard
                                     returns at once while it is set, remove_keyboard clears it */
    /* **Whether the caps lock stays where the game put it.** Bit 0x40 of BIOS
       40:17 is caps lock active, and both reads are about that bit:
       `install_keyboard` sets it when this is set, and the ISR skips the
       toggle for a 0x40 scancode unless this is clear. Read twice, written
       nowhere and zero in the image, so the lock is left to the ISR. */
    uint8_t   hold_caps_lock;     /* +0x01 [1] */
    /* **The last key event the ISR made**, the scancode and the character as
       one word: what it pushes into the BIOS ring, and cleared again on the
       release. */
    uint16_t  last_event;         /* +0x02 [2] */
    /* **The keyboard's tables**, as `keyboard_isr` reads them. The extents
       are the ISR's own bounds - it drops any scancode at or above 0x59
       before touching a table, and walks the PCjr remap eleven wide - and
       the record ends where `ENGINE_PCJR_KEYBOARD` begins. The two pads are bytes nothing
       in the port reads. What `held` holds is a reading: the ISR files the
       scancode's upper bits there on a press and clears the slot on the
       matching release. */
    uint8_t   held[2];            /* +0x04 [2] */
    uint8_t   pad_4592[0x48];     /* +0x06 [0x48] */
    uint8_t   ascii[0x59];        /* +0x4e [0x59]  scancode to character */
    uint8_t   shifted[0x59];      /* +0xa7 [0x59]  the same with shift down */
    uint8_t   state[0x59];        /* +0x100 [0x59]  a bit per key: down */
    uint8_t   pad_46e5[0x20];     /* +0x159 [0x20] */
    uint8_t   pcjr_from[0x0b] NONSTRING;  /* +0x179 [0xb]  the PCjr's scancodes ... */
    uint8_t   pcjr_to[0x0b];      /* +0x184 [0xb]  ... and what they stand for */
} PACKED;

struct engine_keyboard ENGINE_KEYBOARD DGROUP_AT(0x458c) = {
    .pad_4592 = { 0x01 },
    .ascii = {
        0x00, 0x1b, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x2d, 0x3d, 0x08, 0x09, 0x71, 0x77, 0x65, 0x72, 0x74, 0x79,
        0x75, 0x69, 0x6f, 0x70, 0x5b, 0x5d, 0x0d, 0x84, 0x61, 0x73, 0x64,
        0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x3b, 0x27, 0x60, 0x82, 0x5c,
        0x7a, 0x78, 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x2c, 0x2e, 0x2f, 0x81,
        0x2a, 0x88, 0x20, 0xc0,
    },
    .shifted = {
        0x00, 0x1b, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28,
        0x29, 0x5f, 0x2b, 0x08, 0x00, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59,
        0x55, 0x49, 0x4f, 0x50, 0x7b, 0x7d, 0x0d, 0x84, 0x41, 0x53, 0x44,
        0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0x3a, 0x22, 0x7e, 0x82, 0x7c,
        0x5a, 0x58, 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3c, 0x3e, 0x3f, 0x81,
        0x00, 0x88, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x38, 0x39, 0x2d, 0x34, 0x35,
        0x36, 0x2b, 0x31, 0x32, 0x33, 0x30, 0x2e,
    },
    .state = {
        [16] = 0x6e,
        [17] = 0x7e,
        [18] = 0x8e,
        [19] = 0x06,
        [20] = 0x06,
        [21] = 0x06,
        [22] = 0x06,
        [23] = 0x06,
        [24] = 0x06,
        [25] = 0x06,
        [26] = 0x02,
        [27] = 0x02,
        [30] = 0x5e,
        [31] = 0x9e,
        [32] = 0x1e,
        [33] = 0x06,
        [34] = 0x06,
        [35] = 0x06,
        [36] = 0x06,
        [37] = 0x06,
        [38] = 0x06,
        [43] = 0x02,
        [44] = 0x4e,
        [45] = 0x3e,
        [46] = 0x2e,
        [47] = 0x06,
        [48] = 0x06,
        [49] = 0x06,
        [50] = 0x06,
        [71] = 0x60,
        [72] = 0x70,
        [73] = 0x80,
        [75] = 0x50,
        [76] = 0x90,
        [77] = 0x10,
        [79] = 0x40,
        [80] = 0x30,
        [81] = 0x20,
    },
    .pad_46e5 = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x03, 0x00, 0x80, 0xa0, 0x40, 0xe0, 0xc0, 0xf0, 0x60, 0xb0,
        0x00, 0x08, 0x0a, 0x02, 0x06, 0x04, 0x05, 0x01, 0x09,
    },
    .pcjr_from = "YZUT)X+JNVW",
    .pcjr_to = {
        0x57, 0x58, 0x52, 0x46, 0x48, 0x47, 0x4b, 0x50, 0x4d, 0x53, 0x1c,
    },
};

/*
 * **The PCjr keyboard flag**, DGROUP 0x471b..0x471c, 0x01 bytes.
 *
 * `install_keyboard` clears it and then calls `detect_pcjr`; the path that
 * would set it is 0x210f3, which is a stub here. Both readers are PCjr
 * keyboard quirks - one remaps scancode 0x29 to 0x48, the other treats Caps
 * and Num as keys that never report a release.
 */
struct engine_pcjr_keyboard {
    uint8_t   pcjr_keyboard;      /* +0x00 [1] */
} PACKED;

struct engine_pcjr_keyboard ENGINE_PCJR_KEYBOARD DGROUP_AT(0x471b);

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
    if (ENGINE_KEYBOARD.installed == 0) {

        S1C_KEYBOARD.old_int9 = dos_getvect(0x09);
        S1C_KEYBOARD.old_int1c = dos_getvect(0x1c);

        dos_setvect(0x09, (struct far_ptr){ 0x4f46, (uint16_t)(S1C25 >> 4) });

        if (hook_timer != 0)
            dos_setvect(0x1c, (struct far_ptr){ 0x5136, (uint16_t)(S1C25 >> 4) });

        ENGINE_PCJR_KEYBOARD.pcjr_keyboard = 0;

        if (detect_pcjr() != 0)
            not_transcribed("0x210f3, the PCjr keyboard path - INT 15h, the "
                            "keyboard type at 0040:0096, and the remapping "
                            "at 0x2110c");

        ENGINE_KEYBOARD.installed = 1;
    }

    FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) & 0xdf);

    if (ENGINE_KEYBOARD.hold_caps_lock != 0)
        FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) | 0x40);

    return ENGINE_KEYBOARD.installed;
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
    if (ENGINE_KEYBOARD.installed == 0)
        return 0;

    ENGINE_KEYBOARD.installed = 0;

    FAR16(0x40, 0x1A) = FAR16(0x40, 0x1C);

    dos_setvect(0x09, S1C_KEYBOARD.old_int9);
    dos_setvect(0x1c, S1C_KEYBOARD.old_int1c);

    return 1;
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

    if (VMDS.is_pcjr == 1) {
        if (ENGINE_PCJR_KEYBOARD.pcjr_keyboard == 1) {
            if (al == 0x29)
                al = 0x48;
            if (al == 0x2b)
                al = 0x4b;
        } else {
            int16_t i;

            for (i = 0; i < 0xb; i++)
                if (ENGINE_KEYBOARD.pcjr_from[i] == al) {
                    al = ENGINE_KEYBOARD.pcjr_to[i];
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
    dl = ENGINE_KEYBOARD.state[bx];          /* the state as it was */
    dh = (uint8_t)((dh & dl) ^ 1);
    ENGINE_KEYBOARD.state[bx] = dh;

    if (ENGINE_PCJR_KEYBOARD.pcjr_keyboard == 1 && (bx == 0x3a || bx == 0x45))
        al = (uint8_t)bx;                       /* Caps and Num, never a release */

    if ((al & 0x80) != 0) {
        /* ---- a key coming up ---- */
        if ((dl & 0xf8) != 0) {
            cx = (uint16_t)(dl >> 3);
            di = (uint16_t)(cx & 1);
            cl = (uint8_t)(cx >> 1);
            ch = ENGINE_KEYBOARD.held[di];
            if (ch == cl)
                ENGINE_KEYBOARD.held[di] = 0;
        }

        ENGINE_KEYBOARD.last_event = 0;

        al = ENGINE_KEYBOARD.ascii[al & 0x7f];
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
        ENGINE_KEYBOARD.held[di] = (uint8_t)(cx >> 1);
    }

    cl = al;                                    /* the scancode, for AH later */
    di = al;
    al = ENGINE_KEYBOARD.ascii[di];

    if ((al & 0x80) != 0) {
        al &= 0x7f;
        if ((al & 0x70) == 0) {
            FAR8(0x40, 0x17) = (uint8_t)(FAR8(0x40, 0x17) | al);
            io_out8(0x20, 0x20);
            return;
        }
        if ((al & 0x40) == 0 || ENGINE_KEYBOARD.hold_caps_lock == 0) {
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
        al = ENGINE_KEYBOARD.shifted[di];
    }

    {
        uint16_t ax = (uint16_t)((cl << 8) | al);
        uint16_t head, tail;
        int16_t full = 0;

        ENGINE_KEYBOARD.last_event = ax;

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
int16_t key_is_down(uint16_t index)
{
    return (int16_t)(ENGINE_KEYBOARD.state[index] & 1);
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
#endif
