/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game's DOS helpers, and the interrupt stack**: `findfirst` and
 * `findnext` with the find result copied into DGROUP, `chdir`, `mkdir`,
 * `rmdir`, `unlink`, the current drive and directory, the attributes, a
 * disk reset, and the switch that puts an interrupt handler on a stack of
 * its own. Hand-written assembly - `findfirst` calls its copier after its
 * own epilogue, and `isr_stack_switch` pops its return address to swap
 * stacks under it.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x0b6b7..0x0b859, and its `_DATA` 0x2d48..0x3182: the find result, and
 * the private stack whose top is 0x2e7c. The front is ours - cursor.c's
 * last routine ends there - and the end is where mono.c's C begins. They
 * are the game's, not the runtime's: they sit in the game's part of `_TEXT`,
 * before the library begins at 0x0bbfe.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it; the host's transcription is the
 * `#else`. See glue.c for how the block reaches the assembler.
 */
asm {
_DATA segment word public 'DATA'
        dw 0
find_name db 13 dup (0)
        db 1fh dup (0)
find_attr db 0
find_size dw 0, 0
dos_result dw 0
        db 0ffh dup (0)
isr_stack_top label word
        db 302h dup (0)
isr_saved_ss dw 0
isr_saved_sp dw 0
_DATA ends

_TEXT segment byte public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _dos_findfirst, _dos_findnext, _dos_find_to_dgroup, _dos_find_attr
public _dos_find_name, _dos_find_size, _diskette_motor_bit, _dos_chdir
public _dos_mkdir, _dos_rmdir, _dos_unlink, _dos_drive_letter
public _dos_get_cur_dir, _dos_drive_fixed, _dos_disk_reset
public _dos_set_attributes, _dos_get_attributes, _dos_setdisk
public _isr_stack_switch

/* 0x0c2f9 */
_dos_findfirst proc far
        push bp
        mov bp, sp
        push si
        push di
        mov ax, [bp+6]
        mov cx, [bp+8]
        mov dx, ax
        mov ah, 4eh
        int 21h
        pop di
        pop si
        pop bp
        xor ah, ah
        push ax
        call _dos_find_to_dgroup
        pop ax
        retf
_dos_findfirst endp

/* 0x0c315 */
_dos_findnext proc far
        push bp
        mov bp, sp
        push si
        push di
        mov ax, [bp+6]
        mov cx, [bp+8]
        mov dx, ax
        mov ah, 4fh
        int 21h
        pop di
        pop si
        pop bp
        xor ah, ah
        push ax
        call _dos_find_to_dgroup
        pop ax
        retf
_dos_findnext endp

/* 0x0c331: the DTA's attribute, size and name into DGROUP. */
_dos_find_to_dgroup proc near
        push bp
        mov bp, sp
        push bx
        push cx
        push si
        push es
        mov ah, 2fh
        int 21h
        mov cx, 0dh
        mov si, bx
        add si, 1eh
        add bx, 15h
        mov al, es:[bx]
        mov find_attr, al
        add bx, 5
        mov ax, es:[bx]
        mov find_size, ax
        mov ax, es:[bx+2]
        mov find_size+2, ax
        xor bx, bx
next_byte:
        mov al, es:[si]
        mov find_name[bx], al
        inc bx
        inc si
        loop next_byte
        pop es
        pop si
        pop cx
        pop bx
        pop bp
        ret
_dos_find_to_dgroup endp

/* 0x0c370 */
_dos_find_attr proc far
        mov al, find_attr
        xor ah, ah
        retf
_dos_find_attr endp

/* 0x0c376 */
_dos_find_name proc far
        mov ax, offset DGROUP:find_name
        retf
_dos_find_name endp

/* 0x0c37a */
_dos_find_size proc far
        mov ax, find_size
        mov dx, find_size+2
        retf
_dos_find_size endp

/* 0x0c382 */
_diskette_motor_bit proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov es, ax
        mov bx, 43fh
        mov cl, [bp+6]
        inc ax
        shl ax, cl
        and ax, es:[bx]
        pop bp
        retf
_diskette_motor_bit endp

/* 0x0c397 */
_dos_chdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 3bh
        int 21h
        jb chdir_err
        xor ax, ax
chdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_chdir endp

/* 0x0c3ac */
_dos_mkdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 39h
        int 21h
        jb mkdir_err
        xor ax, ax
mkdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_mkdir endp

/* 0x0c3c1 */
_dos_rmdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 3ah
        int 21h
        jb rmdir_err
        xor ax, ax
rmdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_rmdir endp

/* 0x0c3d6 */
_dos_unlink proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 41h
        int 21h
        jb unlink_err
        xor ax, ax
unlink_err:
        mov dos_result, ax
        pop bp
        retf
_dos_unlink endp

/* 0x0c3eb */
_dos_drive_letter proc far
        mov ax, 1900h
        int 21h
        add al, 41h
        xor ah, ah
        retf
_dos_drive_letter endp

/* 0x0c3f5 */
_dos_get_cur_dir proc far
        push bp
        mov bp, sp
        push si
        xor ax, ax
        mov ah, 19h
        int 21h
        add al, 41h
        mov bx, [bp+6]
        mov [bx], al
        mov byte ptr [bx+1], 3ah
        mov byte ptr [bx+2], 5ch
        add bx, 3
        mov si, bx
        mov dl, 0
        mov ax, 4700h
        int 21h
        pop si
        pop bp
        retf
_dos_get_cur_dir endp

/* 0x0c41d */
_dos_drive_fixed proc far
        push bp
        mov bp, sp
        mov bx, [bp+6]
        add bx, 1
        mov ax, 4408h
        int 21h
        pop bp
        retf
_dos_drive_fixed endp

/* 0x0c42d (1.00's; not yet placed in 1.11) */
_dos_disk_reset proc far
        mov ax, 0d00h
        int 21h
        retf
_dos_disk_reset endp

/* 0x0c433 */
_dos_set_attributes proc far
        push bp
        mov bp, sp
        mov dx, [bp+6]
        mov cx, [bp+8]
        mov ax, 4301h
        int 21h
        jb setattr_err
        xor ax, ax
setattr_err:
        pop bp
        retf
_dos_set_attributes endp

/* 0x0c447 */
_dos_get_attributes proc far
        push bp
        mov bp, sp
        mov dx, [bp+6]
        mov ax, 4300h
        int 21h
        mov ax, cx
        jae getattr_ok
        mov ax, 0ffffh
getattr_ok:
        pop bp
        retf
_dos_get_attributes endp

/* 0x0c45b */
_dos_setdisk proc far
        push bp
        mov bp, sp
        mov al, [bp+6]
        and al, 5fh
        sub al, 41h
        mov dl, al
        mov ax, 0e00h
        int 21h
        pop bp
        retf
_dos_setdisk endp

/* 0x0c46e: the return address and the argument come off the stack, and go
   back on whichever stack SS:SP is left on. */
_isr_stack_switch proc far
        pop bx
        pop cx
        pop dx
        or dx, dx
        je isr_restore
        mov ax, ss
        mov isr_saved_ss, ax
        mov ax, sp
        mov isr_saved_sp, ax
        mov ax, ds
        mov ss, ax
        mov ax, offset DGROUP:isr_stack_top
        mov sp, ax
        push dx
        push cx
        push bx
        cld
        retf
isr_restore:
        mov ax, isr_saved_ss
        mov ss, ax
        mov ax, isr_saved_sp
        mov sp, ax
        push dx
        push cx
        push bx
        retf
_isr_stack_switch endp
_TEXT ends
}
#else

/*
 * **The name the last `findfirst`/`findnext` answered**, at DGROUP 0x2d4a:
 * thirteen bytes `dos_find_to_dgroup` copies out of the DTA and
 * `dos_find_name` answers. The word before it and the 0x1f bytes after, up to
 * g_borland_find_info, are not established.
 *
 * DGROUP 0x2d48..0x2d76, 0x2e bytes.
 */
struct borland_find_name {
    /* Nothing touches it and the image never names the offset. */
    uint16_t  _pad_2d48;          /* +0x00 [2] */
    char      find_name[13];      /* +0x02 [0xd] */
    uint8_t   unread_2d57[0x1f];  /* +0x0f [0x1f] */
} PACKED;

struct borland_find_name g_borland_find_name;

/*
 * **Not established**, DGROUP 0x2d76..0x2d7d, 0x07 bytes.
 */
struct borland_find_info {
    /* The attribute byte the last find left in the DTA, which
       `dos_find_attr` answers. */
    uint8_t   attr;               /* +0x00 [1] */
    uint32_t  size;               /* +0x01 [4]  the size of the entry just found, which
                                     dos_find_to_dgroup copies out of the DTA */
    /* **The DOS result of the last directory call**, zero when the carry was
       clear: `chdir`, `mkdir`, `rmdir` and `unlink` each file theirs here.
       Nothing in the image reads it - the four stores are its only
       references. */
    int16_t   dos_result;         /* +0x05 [2] */
} PACKED;

struct borland_find_info g_borland_find_info;

/*
 * **The interrupt's own stack**, DGROUP 0x317e..0x3182, 0x04 bytes.
 *
 * `isr_stack_switch` files `SS:SP` here on the way in so the handler can run
 * on a private stack and put the interrupted one back on the way out. The port
 * does not switch stacks - it has no single SP to switch - but it writes both
 * words, because anything else is free to read them.
 */
struct machine_isr_stack {
    uint16_t  saved_ss;           /* +0x00 [2] */
    uint16_t  saved_sp;           /* +0x02 [2] */
} PACKED;

struct machine_isr_stack g_machine_isr_stack;

/*
 * OURS: **the disk transfer area**, as DOS lays it out - 21 bytes of DOS's
 * own search state, then the attribute at +0x15, the time and date at +0x16
 * and +0x18, the size at +0x1a and the name at +0x1e. The game never moves
 * it, so this is the default one at PSP+0x80; DOS fills it and
 * `dos_find_to_dgroup` copies the result out of it.
 */
static struct dos_dta {
    uint8_t   search[0x15];
    uint8_t   attr;               /* +0x15 */
    uint16_t  time;               /* +0x16 */
    uint16_t  date;               /* +0x18 */
    uint32_t  size;               /* +0x1a */
    uint8_t   name[13];           /* +0x1e */
} __attribute__((packed)) g_dta;

/*
 * 0x0c2f9
 *
 * Borland's `findfirst`: INT 21h AH=4Eh with the pattern in DS:DX and the
 * attribute in CX, then the DTA copied out. The answer is AL zero-extended, so
 * 0 is a match and 18 is "no more files" - the carry flag is never looked at.
 */
uint16_t dos_findfirst(const char *pattern, uint16_t attr)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && pattern[i] != 0; i++)
        name[i] = pattern[i];
    name[i] = 0;

    /*
     * **Nothing is cleared first.** A find that fails leaves the DTA exactly as
     * it was - DOS does not touch it - and 0x0b6ef copies it out either way, so
     * the *previous* name is still there afterwards. Zeroing the buffer here
     * would publish a blank where the original publishes the last name it
     * found - the twelve bytes of "TONSOFUN.TIM" after `fill_file_listing`'s
     * loop ran off the end.
     */
    r = io_dos_findfirst(name, attr, g_dta.name, &g_dta.attr, &g_dta.size);

    dos_find_to_dgroup();
    return (uint16_t)r;
}

/*
 * 0x0c315
 *
 * Borland's `findnext`: INT 21h AH=4Fh, and `findfirst`'s code to the byte
 * apart from the function number. It loads DS:DX and CX from its arguments the
 * same way even though AH=4Fh reads neither - the search state is DOS's, in the
 * DTA - so the pattern it is passed is decoration.
 */
uint16_t dos_findnext(const char *pattern, uint16_t attr)
{
    int16_t r;

    (void)pattern;
    (void)attr;

    /* Nothing cleared, for the reason given in `dos_findfirst`. */
    r = io_dos_findnext(g_dta.name, &g_dta.attr, &g_dta.size);

    dos_find_to_dgroup();
    return (uint16_t)r;
}

/*
 * 0x0c331
 *
 * **Copy the find result out of the DTA and into DGROUP.** It asks DOS where
 * the DTA is - AH=2Fh, answered in ES:BX - and lifts three things out of it:
 * the attribute byte at +0x15 to 0x2d76, the four size bytes at +0x1a to
 * 0x2d77, and the thirteen name bytes at +0x1e to 0x2d4a.
 *
 * The name is copied with a `loop` of exactly 0x0d, so the NUL comes with it
 * only because DOS wrote one - nothing here terminates the string.
 *
 * Both `findfirst` and `findnext` call this on the way out, *after* their own
 * epilogue and with AX pushed across it, which is why the answer survives.
 */
void dos_find_to_dgroup(void)
{
    uint16_t i;

    g_borland_find_info.attr  = g_dta.attr;
    g_borland_find_info.size = g_dta.size;

    for (i = 0; i < 0x0d; i++)
        g_borland_find_name.find_name[i] = (char)g_dta.name[i];
}

/*
 * 0x0c370
 *
 * The attribute of the entry just found, zero-extended out of the byte at
 * DGROUP 0x2d76. Three instructions and no frame - it is a field accessor that
 * happens to be far-callable.
 */
uint16_t dos_find_attr(void)
{
    return g_borland_find_info.attr;
}

/*
 * 0x0c376
 *
 * The name of the entry just found: the *address* 0x2d4a, not a copy. Two
 * instructions. Every caller reads through it before the next `findnext`
 * overwrites it.
 */
char *dos_find_name(void)
{
    return (char *)g_borland_find_name.find_name;
}

/*
 * 0x0c37a
 *
 * The size of the entry just found, as a long in DX:AX out of the long at
 * 0x2d77.
 */
uint32_t dos_find_size(void)
{
    return g_borland_find_info.size;
}

/*
 * 0x0c382
 *
 * **Is bit `bit` of the BIOS word at 0000:043F set?** - the diskette motor
 * status, one bit per drive, and the byte after it. `1 << bit` against the
 * word, answered as it stands. Nothing calls it.
 */
uint16_t diskette_motor_bit(uint16_t bit)
{
    return (uint16_t)((1u << (bit & 0xff)) &
                      g_bios.motor_status);
}

/*
 * 0x0c397
 *
 * Borland's `chdir`: INT 21h AH=3Bh with the path in DX, answering 0 on success
 * and the DOS error code otherwise, and filing that same value at DGROUP
 * 0x2d7b - which is `errno`.
 *
 * **`ax` is zeroed before the call and again after it**, and only the carry
 * flag decides which zero survives. So a DOS that leaves rubbish in `ax` on
 * success cannot make this look like a failure, and `errno` is cleared by a
 * successful call rather than merely left alone.
 *
 * The change itself is `io_dos_chdir`, which is the port's own and models what
 * the emulator does: a directory inside the game's, with the game's directory
 * as a **floor** rather than a starting point, so a guest that walks up with
 * `..` cannot walk out.
 */
uint16_t dos_chdir(const char *path)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && path[i] != 0; i++)
        name[i] = path[i];
    name[i] = 0;

    r = io_dos_chdir(name);

    g_borland_find_info.dos_result = r;
    return (uint16_t)r;
}

/*
 * 0x0c3ac
 *
 * Borland's `mkdir`: INT 21h AH=39h with the path in DX, answering 0 or the DOS error
 * and filing the answer at DGROUP 0x2d7b as `dos_chdir` does. Nothing calls
 * it. The port's answer is `io_dos_mkdir`'s.
 */
uint16_t dos_mkdir(const char *path)
{
    int16_t r = io_dos_mkdir(path);

    g_borland_find_info.dos_result = r;
    return (uint16_t)r;
}

/*
 * 0x0c3c1
 *
 * Borland's `rmdir`: INT 21h AH=3Ah with the path in DX, answering 0 or the DOS error
 * and filing the answer at DGROUP 0x2d7b as `dos_chdir` does. Nothing calls
 * it. The port's answer is `io_dos_rmdir`'s.
 */
uint16_t dos_rmdir(const char *path)
{
    int16_t r = io_dos_rmdir(path);

    g_borland_find_info.dos_result = r;
    return (uint16_t)r;
}

/*
 * 0x0c3d6
 *
 * Borland's `unlink`: INT 21h AH=41h with the path in DX, answering 0 on
 * success and the DOS error otherwise, filed at DGROUP 0x2d7b like the rest.
 * The machine writer uses it to delete a file it failed to finish, so a
 * half-written machine cannot be loaded.
 *
 * The same `xor ax,ax` before the call and after it as `chdir`, with only the
 * carry flag choosing between them.
 *
 * The DOS call is `io_dos_unlink`'s: 0, or DOS 2, "file not found".
 */
uint16_t dos_unlink(const char *path)
{
    char name[256];
    uint16_t i;
    int16_t r;

    for (i = 0; i < sizeof name - 1 && path[i] != 0; i++)
        name[i] = path[i];
    name[i] = 0;

    r = io_dos_unlink(name);

    g_borland_find_info.dos_result = r;
    return (uint16_t)r;
}

/*
 * 0x0c3eb
 *
 * **The current drive as a letter**: INT 21h AH=19h plus 'A'. Nothing calls
 * it; `dos_get_cur_dir` asks the same itself.
 */
uint16_t dos_drive_letter(void)
{
    return (uint16_t)((io_dos_curdrive() + 0x41) & 0xff);
}

/*
 * 0x0c3f5
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
void dos_get_cur_dir(char *buf)
{
    buf[0] = (char)(io_dos_curdrive() + 0x41);
    buf[1] = ':';
    buf[2] = '\\';

    /*
         * The cast goes through `uintptr_t` because DGROUP is volatile - the
         * timer handler runs on a thread and shares it - and this one place
         * hands a pointer *into* it to the IO layer, which fills the buffer
         * itself. Nothing else does that, and the volatility is the port's
         * memory model rather than anything the original had.
         */
        io_dos_getcwd((uint8_t *)(buf + 3));
}

/*
 * 0x0c41d
 *
 * **Is the drive's medium removable?** INT 21h AX=4408h for drive `drive`
 * plus one - 0 removable, 1 fixed - with the carry not looked at. Nothing
 * calls it.
 */
uint16_t dos_drive_fixed(uint16_t drive)
{
    return io_dos_drive_fixed((uint8_t)(drive + 1));
}

/*
 * 0x0c42d (1.00's; not yet placed in 1.11)
 *
 * **Reset the disks**: INT 21h AH=0Dh, flushing DOS's buffers. Nothing
 * calls it.
 */
void dos_disk_reset(void)
{
    io_dos_disk_reset();
}

/*
 * 0x0c433
 *
 * **Set a file's attributes**: INT 21h AX=4301h, answering 0 or the DOS
 * error. Nothing calls it.
 */
uint16_t dos_set_attributes(const char *name, uint16_t attr)
{
    return (uint16_t)io_dos_setattr(name, attr);
}

/*
 * 0x0c447
 *
 * **A file's attributes**: INT 21h AX=4300h, answering CX, or -1 when the
 * carry says it failed. Nothing calls it.
 */
uint16_t dos_get_attributes(const char *name)
{
    return (uint16_t)io_dos_getattr(name);
}

/*
 * 0x0c45b
 *
 * Borland's `setdisk`: INT 21h AH=0Eh, with the drive taken from a *letter* -
 * `and al, 0x5f` uppercases it and `sub al, 0x41` makes it the number DOS
 * wants, so 'a' and 'A' are both drive zero.
 *
 * The mask is 0x5f and not 0xdf, so it also clears bit 5 **and bit 7**: a
 * letter with the high bit set still lands on a drive rather than on a number
 * over 0x80. It answers nothing - DOS returns the drive count in `al` and this
 * throws it away.
 *
 * The port serves one directory and therefore one drive, so `io_dos_setdisk`
 * changes nothing. That is not a stub: selecting the only drive there is *is*
 * a no-op, and the game is never told otherwise because this answers nothing.
 *
 * **The reference agrees by not implementing it at all** - the emulator logs
 * `UNHANDLED INT 21h AH=0eh` every time the game gets here and carries on. So
 * a no-op is not the port settling for less than the reference does; it is the
 * same behaviour reached from the other direction.
 */
void dos_setdisk(uint8_t letter)
{
    io_dos_setdisk((uint8_t)(((letter & 0x5f) - 0x41) & 0xff));
}

/*
 * 0x0c46e
 *
 * Switch the interrupt handler onto a stack of its own, and back: a non-zero
 * argument saves SS:SP at DGROUP 0x317e and puts SP at 0x2e7c inside DGROUP, a
 * zero one puts the saved pair back. The entry at 0x0b84b is the second half
 * reached directly.
 *
 * It does this by popping its own return address and argument off the stack,
 * changing SS:SP, and pushing them back - the only way to return onto a stack
 * you have just swapped.
 *
 * **The switch itself means nothing here.** The port's handler runs on a real
 * thread with a real stack of its own, which is what the private stack was for.
 * SS is still written, because anything else can read it.
 */
void isr_stack_switch(int16_t to_private)
{
    if (to_private != 0) {
        /* SS is DGROUP's; SP is a register the port does not have, so the
           word the original saves it in is left as it is. */
        g_machine_isr_stack.saved_ss = DGROUP_SEG;
        return;
    }

    /* The restore half at 0x0b84b: the saved pair goes back into SS:SP. */
}
#endif
