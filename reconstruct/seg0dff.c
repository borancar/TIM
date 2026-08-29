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
    game_intro();
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

    DGU16(0x52f6) = load_bitmap_list(0x00eb);          /* "mouse.bmp"   */
    DGU16(0x52f4) = load_bitmaps(0x00f5);          /* "cp.bmp"      */
    DGU16(0x4ecb) = load_bitmaps(0x00fc);          /* "gp_bord.bmp" */

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

    select_cursor(0);
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
 * The intros: the Sierra logo, then the title screen and the credits, looping
 * between the last two until a key or a mouse button ends it. `main` calls this
 * second, after the bring-up.
 *
 * **The Sierra logo** is a table of six-byte entries at DGROUP 0x2370 - an x, a
 * y offset and a bitmap index - walked one entry a frame with a rectangle
 * cleared behind each. DGROUP 0x44ef is a frame budget that starts at 0x2710
 * and is compared against as the animation runs, so a slow machine drops
 * entries rather than falling behind. The `add ax, 0xff88` that sets the limit
 * is a subtraction of 0x78 written as an addition, which is what the compiler
 * does with a negative constant.
 *
 * **The title loop** is the shape worth reading. DGROUP 0x4e6b is a state, and
 * 0x8000 means the title and anything else the credits, chosen by which .gkc
 * file is loaded and remembered in the local at [bp-0xa]. Each pass draws,
 * presents, polls, and counts DGROUP 0x4ea7 up; at 0x110 frames for the title
 * or 0x152 for the credits the pass ends and the other one starts. A key or a
 * button - which is what makes DGROUP 0x5772 or 0x5774 become 2 - drops
 * straight out.
 *
 * **The pages swap roles twice.** For the logo both 0x38a2 and 0x38a4 are
 * A000, so the logo is drawn and shown on the same page; the game's screens
 * then go back to the usual A000/A820 pair, and the last lines set 0x38a4 to
 * 0xa190 and 0x38a2 to 0xa8c0 - the two pages offset by 0x190 paragraphs, which
 * is the 400-line screen sitting inside the 480-line mode.
 */
uint16_t game_intro(void)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t name = fp;                     /* [bp-0xe] is a word, not the buf */
    uint16_t bitmaps;                       /* [bp-0xc] */
    uint16_t gkc;                           /* [bp-0xe] */
    int16_t stage;                          /* [bp-4]  */
    int16_t budget;                         /* [bp-2]  */
    int16_t which;                          /* [bp-0xa] */
    int16_t frame;                          /* [bp-8]  */
    int16_t running;                        /* [bp-6]  */
    uint16_t di;
    int16_t si;

    (void)name;

    DG16(0x44ef) = 0x2710;

    set_palette_pointer(DGU16(0x52e1), DGU16(0x52e3));      /* black.pal */

    bitmaps = load_bitmaps(0x254a);                         /* "sierra.bmp" */

    DGU16(0x38a2) = 0xa000;
    DGU16(0x38a4) = 0xa000;

    for (si = 0; si < 3; si++)
        present_frame(1);

    DGU16(0x38a2) = (uint16_t)(DGU16(0x38a2) + 0x12c);
    DGU16(0x4e6b) = 0x8000;
    DG16(0x52d5) = -1;

    stage = 0;
    di = 0x2370;

    for (;;) {
        if (stage == 0) {
            DGU16(0x38a8) = DGU16(0x38a4);
            clear_flag_2d44_thunk();
            load_screen(0x2555);                              /* "sierra.scr" */
            set_palette_pointer(DGU16(0x52e5), DGU16(0x52e7));  /* sierra.pal */
            stage = 1;
            budget = (int16_t)(DG16(0x44ef) + 0xff88);
            di = 0x2370;
        }

        /*
         * `jl` - the step runs while the counter is *under* the budget, and
         * the budget is set 0x78 below whatever the counter was. So nothing
         * moves until DGROUP 0x44ef counts down, which is the timer's doing:
         * this is the frame pacing, not a frame counter.
         */
        if (DGU16(di) != 0 && (int16_t)(DG16(0x44ef) + 6) < budget) {
            clip_enabled = 1;
            clip_top = 0;
            clip_left = 0;
            clip_right = 0x27f;
            clip_bottom = 0x1df;
            fill_enabled = 1;
            vga_fill_colour = 0;
            vga_second_colour = 0;

            DGU16(0x38a8) = DGU16(0x38a2);
            fill_rect(0x1c0, 0x19f, 0xc0, 0x41);

            draw_bitmap(DGU16((uint16_t)(bitmaps
                                         + 2 * DG16((uint16_t)(di + 4)))),
                        (int16_t)DGU16(di),
                        (int16_t)(DG16((uint16_t)(di + 2)) + 0x19f), 0);

            if (DG16((uint16_t)(di + 4)) == 0)
                sub_083ab(0x14);

            di = (uint16_t)(di + 6);

            draw_bitmap(DGU16((uint16_t)(bitmaps
                                         + 2 * DG16((uint16_t)(di + 4)))),
                        (int16_t)DGU16(di),
                        (int16_t)(DG16((uint16_t)(di + 2)) + 0x19f), 0);

            di = (uint16_t)(di + 6);

            DGU16(0x38a8) = DGU16(0x38a4);
            DGU16(0x38a6) = DGU16(0x38a2);
            copy_rect_thunk(0x1c0, 0x1a9, 0xc0, 0x4b);

            budget = DG16(0x44ef);

            if (DGU16(di) == 0) {
                sub_083ab(0x13);
                stage = 4;
            }
        }

        regions_handle_pointer(DGU16(0x4e79));
        update_button_state();

        if (DGU16(0x52fa) != 0)
            sub_0e34a(1);

        if (stage == 4 || DGU16(0x4e6b) != 0x8000)
            break;
    }

    free_bitmaps_thunk(bitmaps);

    DGU16(0x52dd) = 0;
    DGU16(0x52d9) = 0;
    DGU16(0x52db) = 0x27f;
    DGU16(0x52d7) = 0x18f;

    sub_0f7b6();

    gkc = load_bitmaps(0x2560);                             /* "corners.bmp" */

    for (si = 0x37; si <= 0x39; si++)
        sub_0f7f4((uint16_t)si);

    set_palette_pointer(DGU16(0x52e1), DGU16(0x52e3));      /* black.pal */

    DGU16(0x38a4) = 0xa000;
    DGU16(0x38a2) = 0xa820;

    for (si = 0; si < 3; si++)
        present_frame(1);

    DGU16(0x38a8) = 0xa000;
    vm_set_display_lines(0x18f);
    update_button_state();

    if (DGU16(0x5774) == 2 || DGU16(0x5772) == 2) {
        DGU16(0x4e6b) = 2;
        which = 2;
    }

    frame = 0x3f6;

    if (DGU16(0x4e6b) == 0x8000) {
        which = (int16_t)0x8000;
        DGU16(0x4e6b) = 0x2000;
    } else {
        DGU16(0x4e6b) = 2;
        which = 2;
    }

    while ((uint16_t)which == 0x8000 || (uint16_t)which == 0x4000) {
        clear_flag_2d44_thunk();

        if ((uint16_t)which == 0x8000) {
            sub_12915(0x256c);                              /* "title.gkc"   */
            frame = -8;
        } else {
            sub_12915(0x2576);                              /* "credits.gkc" */
            frame = 0x41;
        }

        DG16(0x4ea3) = -0x10;
        DG16(0x4e9f) = -0x10;
        DG16(0x4e9b) = -0x10;
        DG16(0x4ea1) = 0;
        DG16(0x4e9d) = 0;
        DG16(0x4e99) = 0;

        sub_013e9();
        set_clip_full_screen();

        DGU16(0x38a8) = DGU16(0x38a2);
        vga_fill_colour = (uint8_t)DG8(0x52cb);
        vga_second_colour = (uint8_t)DG8(0x52cb);
        fill_enabled = 1;

        fill_rect(0, 0, 0x280, 0x190);

        sub_16181(1);
        sub_0ee6e(gkc);
        present_frame(1);

        DGU16(0x38a6) = DGU16(0x38a4);
        DGU16(0x38a8) = DGU16(0x38a2);
        sub_0b28e(0, 0, 0x280, 0x190);

        sub_08364((uint16_t)(((uint16_t)which == 0x8000) ? 0x3e9 : frame));

        running = 1;

        while (running != 0) {
            if (DGU16(0x52d3) != 0) DGU16(0x52d3) = 1;
            if (DGU16(0x52d1) != 0) DGU16(0x52d1) = 1;
            if (DGU16(0x52cf) != 0) DGU16(0x52cf) = 1;
            if (DGU16(0x52cd) != 0) DGU16(0x52cd) = 1;

            update_button_state();
            sub_00f86();
            sub_06806();
            step_word_4e87();
            sub_06699();

            sub_16181(0);
            sub_0ee6e(gkc);
            present_frame(1);

            if (DGU16(0x4ea7) == 0)
                set_palette_pointer(DGU16(0x52ed), DGU16(0x52ef));  /* tim.pal */

            if (DGU16(0x52d3) == 1) sub_083ea(1);
            if (DGU16(0x52d1) == 1) sub_083ea(2);
            if (DGU16(0x52cf) == 1) sub_083ea(9);
            if (DGU16(0x52cd) == 1) sub_083ea(0xc);

            shift_all_histories();

            if (DGU16(0x5774) == 2 || DGU16(0x5772) == 2) {
                DGU16(0x4e6b) = 2;
                which = 2;
                running = 0;
            }

            DGU16(0x4ea7)++;

            if ((uint16_t)which == 0x8000) {
                if ((int16_t)DGU16(0x4ea7) > 0x110)
                    running = 0;
            } else if ((int16_t)DGU16(0x4ea7) > 0x152) {
                running = 0;
            }
        }

        splice_list_4e58_onto_4e56();
        sub_07e45();

        for (si = 1; si <= 0x14; si++)
            sub_083ea((uint16_t)si);

        sub_14d43();

        if ((uint16_t)which == 0x8000) {
            which = (int16_t)0x4000;
        } else if ((uint16_t)which == 0x4000) {
            which = (int16_t)0x8000;
            frame++;
            if (frame > 0x3f8)
                frame = 0x3ea;
        }
    }

    for (si = 0x37; si <= 0x39; si++)
        sub_0f886((uint16_t)si);

    DGU16(0x4ec7) = load_bitmaps(0x2582);                   /* "icons.bmp" */
    DGU16(0x4e6b) = 0x8000;

    sub_0ea39(gkc);

    DGU16(0x4e6b) = 2;

    set_palette_pointer(DGU16(0x52e1), DGU16(0x52e3));      /* black.pal */
    present_frame(1);

    free_bitmaps_thunk(gkc);

    sub_083ea(0);
    sub_0810b();

    DGU16(0x38a4) = 0xa190;
    DGU16(0x38a2) = 0xa8c0;
    DG16(0x3f7c) = 0x16f;

    vm_set_display_lines(0x1bf);
    sub_08f27(0x16f);

    for (si = 0; si < 3; si++)
        present_frame(1);

    DGU16(0x52dd) = 8;
    DGU16(0x52db) = 0x237;
    DGU16(0x52d9) = 8;
    DGU16(0x52d7) = 0x167;

    dg_leave(0x0e);
    return 0;
}

/*
 * 0x0ea39
 *
 * NOT TRANSCRIBED YET. Called once with the bitmap list, after icons.bmp is loaded.
 */
void sub_0ea39(uint16_t a)
{
    (void)a;
    not_transcribed("0x0ea39");
}

/*
 * 0x0edf1
 *
 * NOT TRANSCRIBED YET. Called from the intro.
 */
void sub_0edf1(uint16_t a, uint16_t b)
{
    (void)a;
    (void)b;
    not_transcribed("0x0edf1");
}

/*
 * 0x0ee6e
 *
 * NOT TRANSCRIBED YET. Called twice a frame with the intro's bitmap list.
 */
void sub_0ee6e(uint16_t a)
{
    (void)a;
    not_transcribed("0x0ee6e");
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
 * 0x0f7b6
 *
 * NOT TRANSCRIBED YET. Called once, between the Sierra logo and corners.bmp.
 */
void sub_0f7b6(void)
{
    not_transcribed("0x0f7b6");
}

/*
 * 0x0f7f4
 *
 * NOT TRANSCRIBED YET. Called with 0x37, 0x38 and 0x39.
 */
void sub_0f7f4(uint16_t a)
{
    (void)a;
    not_transcribed("0x0f7f4");
}

/*
 * 0x0f886
 *
 * NOT TRANSCRIBED YET. Called with 0x37, 0x38 and 0x39, at the end.
 */
void sub_0f886(uint16_t a)
{
    (void)a;
    not_transcribed("0x0f886");
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
 * 0x12915
 *
 * NOT TRANSCRIBED YET. Called with "title.gkc" and with "credits.gkc".
 */
void sub_12915(uint16_t a)
{
    (void)a;
    not_transcribed("0x12915");
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
/*
 * 0x14d43
 *
 * NOT TRANSCRIBED YET. Called once a frame in the intro's loop.
 */
void sub_14d43(void)
{
    not_transcribed("0x14d43");
}

