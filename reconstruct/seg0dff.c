/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 0dff**, image
 * 0x0dff0..0x14de0. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x0dfff
 *
 * **`main`.** The Borland startup calls it at image 0x00155 with argc, argv
 * and envp, and pushes the answer straight into `exit`. The game reads none of
 * the three, which is what a DOS game with no command line looks like.
 *
 * Four calls and nothing else: bring the machine up, then three routines that
 * are not transcribed yet. The last one's result is left in AX and becomes the
 * program's exit status, so it is written here as a `return`; the bytes cannot
 * distinguish that from a bare call whose answer happened to survive.
 */
uint16_t game_main(void)
{
    game_startup();
    sub_0e4be();
    sub_0eed5();
    return sub_0e34a(1);
}

/*
 * 0x0e01d
 *
 * The whole bring-up, in the original's order: refuse to run without enough
 * memory, read the two configuration files, start the video driver, load the
 * palettes, the font and the first bitmaps, start sound, install the timer,
 * and build two free lists.
 *
 * The memory check is a signed 32-bit compare against 0x44d90 - 282,000 bytes
 * - written as a high-word signed test and a low-word unsigned one, which is
 * how the compiler emits `long < constant`. `dos_alloc_bytes` is asked for
 * 0xffffffff bytes with flags 0, which is the "how much is free" call rather
 * than an allocation.
 */
void game_startup(void)
{
    /*
     * The original's 0x14 bytes of locals. Only one of them needs to live in
     * DGROUP - the byte at [bp-1], whose address is handed to `stdio_fread` -
     * but the whole frame is reserved so the port's stack use matches the
     * original's, and [bp-1] is its last byte.
     */
    uint16_t fp = dg_enter(0x14);
    uint16_t cfg_byte = (uint16_t)(fp + 0x13);

    int32_t free_bytes;
    int16_t sound_device, sound_module, cfg_first;
    uint16_t file, i;

    DGU16(0x52fc) = 0x800;

    free_bytes = (int32_t)dos_alloc_bytes(0xffff, 0xffff, 0, 0);
    if (free_bytes < 0x00044d90L) {
        stdio_printf(0x1bcc);       /* "\n\nNOT ENOUGH FREE MEMORY\n" */
        stdio_printf(0x1be6);       /* "\nYou need at least 550k ..."  */
        stdio_exit(0);
    }

    dos_get_cur_dir(0x535b);
    dos_get_cur_dir(0x530b);
    set_holiday_flags();

    DGU16(0x52fa) = 0;
    DGU16(0x4e85) = 0;
    DGU16(0x4ec5) = 0xffff;

    load_archive_map();

    /* What RESOURCE.CFG would have said, if it is not there. */
    cfg_first = 0;
    sound_module = -2;
    sound_device = 0;

    file = stdio_fopen(0x00aa, 0x00b7);         /* "RESOURCE.CFG", "rb" */
    if (file != 0) {
        stdio_fread(cfg_byte, 1, 1, file);
        cfg_first = DGS8(cfg_byte);
        stdio_fread(cfg_byte, 1, 1, file);
        sound_device = DGS8(cfg_byte);
        stdio_fread(cfg_byte, 1, 1, file);
        sound_module = DGS8(cfg_byte);
        stdio_fclose(file);
    }
    (void)cfg_first;    /* the original stores it and never reads it back */

    if (read_tim_cfg() == 0) {
        DGU16(0x4eb7) = 1;
        DGU16(0x4ec1) = 6;
    }

    DGU16(0x4eb5) = 0;
    DGU16(0x4eab) = 0;
    DGU16(0x4ea9) = 0;
    DGU16(0x52cb) = 3;
    DGU16(0x52c9) = 0x0b;

    if (vm_init(0x0d, 0x80, 0x00ba) == 0) {     /* "vm.ovl" */
        stdio_printf(0x1c30);                   /* "Unable to initialize vm." */
        stdio_exit(0);
    }

    DGU16(0x38a4) = 0xa000;
    DGU16(0x38a2) = 0xa820;
    vm_set_display_lines(0x1d6);                /* 470 - the Sierra logo */

    DG32(0x52ed) = (int32_t)load_palette(0x00c1);   /* "tim.pal"    */
    DG32(0x52e5) = (int32_t)load_palette(0x00c9);   /* "sierra.pal" */
    {
        uint32_t black = load_palette(0x00d4);      /* "black.pal"  */

        DG32(0x52e1) = (int32_t)black;
        set_palette_pointer((uint16_t)black, (uint16_t)(black >> 16));
    }

    DGU16(0x52df) = load_font(0x00de);          /* "memofnt8.fnt" */
    set_font((int16_t)DGU16(0x52df));

    DGU16(0x52f6) = sub_2367c(0x00eb);          /* "mouse.bmp"   */
    DGU16(0x52f4) = sub_24f72(0x00f5);          /* "cp.bmp"      */
    DGU16(0x4ecb) = sub_24f72(0x00fc);          /* "gp_bord.bmp" */

    install_keyboard(0);

    start_sound(sound_device, sound_module, 0, 0x0108);     /* "sx.ovl" */

    DGU16(0x52f8) = open_file_record(0x010f);   /* "tim.sx" */
    for (i = 1; i <= 0x14; i++)
        open_sound_file(DGU16(0x52f8), (int16_t)i);

    /* A word table at DGROUP 0x116, indexed by what TIM.CFG put at 0x4ec1. */
    set_master_level_ok(DGU16((uint16_t)(0x116 + DGU16(0x4ec1) * 2)));

    install_divide_trap();
    timer_install(0x0d);
    mouse_init();
    mouse_move_to(10, 10);
    timer_add_callback(0xa7ae, (uint16_t)(IMAGE_BASE >> 4), 4);

    sub_0467d(0);
    erase_both_pages();
    mouse_set_speed(3);
    build_screen_regions();
    count_level_files();

    /*
     * Twenty eight-byte records off the near heap, chained through their first
     * word. 0x4e56 is the head; 0x4e58 is cleared with it and left alone.
     */
    DGU16(0x4e58) = 0;
    DGU16(0x4e56) = 0;
    for (i = 0; i < 0x14; i++) {
        uint16_t p = heap_calloc_far(1, 8);

        DGU16(p) = DGU16(0x4e56);
        DGU16(0x4e56) = p;
    }

    /*
     * And 180 twenty-four-byte records from DOS, chained the same way but
     * through a *far* pointer - offset at 0x4e4e, segment at 0x4e50, and the
     * link in the first four bytes of each block. 0x4e52/0x4e54 are the second
     * head, cleared here and not filled.
     */
    DGU16(0x4e54) = 0;
    DGU16(0x4e52) = 0;
    DGU16(0x4e50) = 0;
    DGU16(0x4e4e) = 0;
    for (i = 0; i < 0xb4; i++) {
        uint32_t block = dos_alloc_bytes(0x18, 0, 0, 1);
        uint16_t off = (uint16_t)block;
        uint16_t seg = (uint16_t)(block >> 16);

        FARU16(seg, (uint16_t)(off + 2)) = DGU16(0x4e50);
        FARU16(seg, off) = DGU16(0x4e4e);
        DGU16(0x4e50) = seg;
        DGU16(0x4e4e) = off;
    }

    dg_leave(0x14);
}

/*
 * 0x0e34a
 *
 * NOT TRANSCRIBED YET. Called from the frame-presentation routine at 0x081cc
 * when DGROUP 0x52fa is set. It is a large routine - it reserves 0x122 bytes
 * of locals - and reading it is a job of its own.
 */
uint16_t sub_0e34a(uint16_t arg)
{
    (void)arg;
    not_transcribed("0x0e34a");
    return 0;
}
/*
 * 0x0e4be
 *
 * NOT TRANSCRIBED YET. `main`'s second call, after the bring-up.
 */
void sub_0e4be(void)
{
    not_transcribed("0x0e4be");
}

/*
 * 0x0eed5
 *
 * NOT TRANSCRIBED YET. `main`'s third call.
 */
void sub_0eed5(void)
{
    not_transcribed("0x0eed5");
}

/*
 * 0x11d44
 *
 * Look a word up in the table that the **far** pointer at DGROUP 0x546c points
 * at, or answer 0 for the index -1. The table is outside DGROUP - it is in a
 * block DOS handed the program - which is why the port models the guest's
 * whole address space rather than only its data segment.
 */
int16_t lookup_table_546c(int16_t index)
{
    if (index == -1)
        return 0;
    return FAR16(DG_FAR_SEG(0x546C),
                 (uint16_t)(DG_FAR_OFF(0x546C) + (uint16_t)(index * 2)));
}


/*
 * 0x11dd1
 *
 * A far-callable two-byte read: `game_fread(buf, 2, 1, file)`, with the
 * arguments the other way round from `fread`'s own - the file first and the
 * buffer second.
 */
void game_fread_far(uint16_t file, uint16_t buf)
{
    game_fread(buf, 2, 1, file);
}

/*
 * 0x129a8
 *
 * Count the level files, and leave the count at DGROUP 0x4eb9.
 *
 * It builds "l", the number, ".lev" and tries to open it, climbing from 1 until
 * one is missing - so the answer is one *past* the last that opened, and the
 * decrement on the failing try is what turns that back into a count. Each file
 * that opens is closed again immediately; nothing is read.
 *
 * The name is assembled in a stack buffer whose address is passed on, so the
 * port needs a real DGROUP frame for it rather than C locals.
 */
void count_level_files(void)
{
    uint16_t fp = dg_enter(0x18);
    uint16_t name = fp;                         /* [bp-0x18] */
    uint16_t number = (uint16_t)(fp + 0x10);    /* [bp-8]    */
    int16_t done = 0;

    DGU16(0x4eb9) = 1;

    while (done == 0) {
        uint16_t file;

        string_copy(name, 0x2887);              /* "l"    */
        int_to_string((int16_t)DGU16(0x4eb9), number, 10);
        string_concat(name, number);
        string_concat(name, 0x2889);            /* ".lev" */

        file = game_fopen(name, 0x288e);        /* "rb"   */

        if (file != 0) {
            DGU16(0x4eb9)++;
            game_fclose(file);
        } else {
            DGU16(0x4eb9)--;
            done = 1;
        }
    }

    dg_leave(0x18);
}

/*
 * 0x12ba7
 *
 * Read `TIM.CFG`: two words, into DGROUP 0x4eb7 and 0x4ec1. Answers 1 if the
 * file was there and 0 if it was not.
 *
 * The name is the string at DGROUP 0x28bb and the mode the one at 0x28c3. Both
 * reads go through `game_fread_far`, which takes its file first and buffer
 * second, and the file is closed on the success path only - a failed open has
 * nothing to close.
 */
uint16_t read_tim_cfg(void)
{
    uint16_t file = game_fopen(0x28bb, 0x28c3);

    if (file == 0)
        return 0;

    game_fread_far(file, 0x4eb7);
    game_fread_far(file, 0x4ec1);
    game_fclose(file);

    return 1;
}
