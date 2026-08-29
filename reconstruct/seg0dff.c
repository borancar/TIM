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
                play_sound(0x14);

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
                play_sound(0x13);
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

    load_all_parts();

    gkc = load_bitmaps(0x2560);                             /* "corners.bmp" */

    for (si = 0x37; si <= 0x39; si++)
        load_part_bitmap((uint16_t)si);

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
            load_animation(0x256c);                              /* "title.gkc"   */
            frame = -8;
        } else {
            load_animation(0x2576);                              /* "credits.gkc" */
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
        free_part_bitmap((uint16_t)si);

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
 * Load the part bitmaps: 0 to 8, then 9 on its own, then 0x0b to 0x30, then
 * 0x32 on its own. **10 and 0x31 are skipped**, and skipped by being left out
 * of the ranges rather than tested for - there is no part with those numbers.
 */
void load_all_parts(void)
{
    int16_t si;

    for (si = 0; si < 8; si++)
        load_part_bitmap((uint16_t)si);

    load_part_bitmap(9);

    for (si = 0x0b; si < 0x31; si++)
        load_part_bitmap((uint16_t)si);

    load_part_bitmap(0x32);
}

/*
 * 0x0f7f4
 *
 * Load one part's bitmaps: build "part" + the number + ".bmp", read it, and
 * keep the list at DGROUP 0xeba + 0x3a * n - so the parts' records are 0x3a
 * bytes apart and this is the first field of each.
 *
 * The heap is checked either side of the load, and the cursor is pinned across
 * it and released after: a load takes long enough for the pointer to want
 * redrawing, and redrawing it in the middle of one would draw it onto a page
 * that is being rebuilt.
 */
void load_part_bitmap(uint16_t n)
{
    uint16_t fp = dg_enter(0x16);
    uint16_t name = fp;                      /* [bp-0x16] */
    uint16_t number = (uint16_t)(fp + 0x0e); /* [bp-8]    */

    string_copy(name, 0x2625);               /* "part" */
    int_to_string((int16_t)n, number, 10);
    string_concat(name, number);
    string_concat(name, 0x262a);             /* ".bmp" */

    heap_check_or_hang();
    clear_flag_2d44_thunk();

    DGU16((uint16_t)(0x0eba + 0x3a * n)) = load_bitmaps(name);

    restore_cursor_following();
    heap_check_or_hang();

    dg_leave(0x16);
}

/*
 * 0x0f886
 *
 * Give one part's bitmaps back, and clear its slot. A slot that is already
 * empty is left alone.
 */
void free_part_bitmap(uint16_t n)
{
    uint16_t at = (uint16_t)(0x0eba + 0x3a * n);

    if (DGU16(at) == 0)
        return;

    free_bitmaps_thunk(DGU16(at));
    DGU16(at) = 0;
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
 * 0x11d66
 *
 * Make room for `n` parts: a far block of `n * 4` bytes from DOS for the table
 * at DGROUP 0x546c, and then `n` records of 0xa2 bytes off the near heap, one
 * put in each of its slots.
 *
 * The table is a **far** array of near pointers - four bytes an entry where the
 * pointer is two - and the game reaches it through `lookup_table_546c`, which
 * is what makes a part number into a record. Two bytes of every four are not
 * written here and are whatever DOS left in the block.
 */
void alloc_part_table(int16_t n)
{
    uint32_t p = dos_alloc_bytes((uint16_t)(n * 4), 0, 0, 0);
    int16_t si;

    DGU16(0x546e) = (uint16_t)(p >> 16);
    DGU16(0x546c) = (uint16_t)p;

    for (si = 0; si < n; si++)
        FARU16(DGU16(0x546e), (uint16_t)(DGU16(0x546c) + 2 * si)) =
            heap_calloc_far(1, 0xa2);
}

/*
 * 0x11db4
 *
 * Read one byte: `game_fread(buf, 1, 1, file)`, with the file first and the
 * buffer second - the same order round as `game_fread_far` beside it.
 */
void game_fread_byte(uint16_t file, uint16_t buf)
{
    game_fread(buf, 1, 1, file);
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
 * 0x11dec
 *
 * Read a null-terminated string, a byte at a time, and **including** the null:
 * the loop reads first and tests afterwards, so the terminator is stored before
 * the test that stops on it. The buffer has to be big enough for the string the
 * file happens to hold; nothing here bounds it.
 */
void game_fread_string(uint16_t file, uint16_t buf)
{
    for (;;) {
        game_fread_byte(file, buf);
        if (DG8(buf) == 0)
            return;
        buf++;
    }
}

/*
 * 0x11e3f
 *
 * Read one part out of a .gkc. `rec` is one of the 0xa2-byte records
 * `alloc_part_table` made in advance; this fills it from the file and then
 * hands it to its kind's own setup, so a part read off disk ends in the same
 * state as one `make_part` built.
 *
 * Most of it is a flat run of two-byte field reads, with four fields copied
 * from another rather than read - +8 from +0x94, +0x0c from +0x90, +0x12 from
 * +0x92, and the pair +0x42/+0x40 from +0x46/+0x44, which is the same "current
 * position becomes the previous one" that `make_part` ends with.
 *
 * Three things are not flat:
 *
 *  - **The rope.** A non-zero word read just before +0x56 means the part
 *    carries one: 0x38 bytes off the near heap at +0x54, whose +2 points back
 *    at the part and whose +4 and +6 are the two parts it ties together, each
 *    stored in the file as a part number and resolved through
 *    `lookup_table_546c`. Each end that exists is pointed back at the rope
 *    through its own +0x54.
 *
 *  - **Two belts**, at +0x66 and +0x68. Each one present is 0x2c bytes off the
 *    near heap naming the two parts it runs between - at +2 and +4, copied
 *    again to +6 and +8 - and, in the bytes at +0x0a and +0x0b, which of each
 *    part's two belt slots it occupies, those two also copied to +0x0c and
 *    +0x0d. Each of those parts is pointed back at the belt through that slot.
 *    The pair of slot bytes is read into the part at +0x6a + 2i and +0x6b + 2i
 *    first, and the belt's own copies are read separately afterwards.
 *
 *  - **The version gate**, the word at DGROUP 0x5474. From 0x101 the file
 *    carries the field at +0x0a and four more part numbers into +0x62..+0x68;
 *    below it, a count and that many pairs of bytes are read and dropped on
 *    the floor. Either way the first two slots at +0x5a and +0x5c are read,
 *    and each is stored **twice**, into +0x5e and +0x60 as well.
 *
 * Kind 7 gets one extra part number, and takes that part's first belt as its
 * own second.
 *
 * The last two fields are not read at all: the count at +0x80 comes from the
 * kind's record at DGROUP 0xec4 + 0x3a * kind and the slots at +0x82 are
 * allocated from it, exactly as `part_init` does, before the far pointer at
 * +0x2a of the same record runs.
 */
void read_record_fields(uint16_t file, uint16_t rec)
{
    uint16_t fp = dg_enter(0x10);
    uint16_t v10 = (uint16_t)(fp + 0x00);       /* [bp-0x10] */
    uint16_t v0e = (uint16_t)(fp + 0x02);       /* [bp-0x0e] */
    uint16_t v0b = (uint16_t)(fp + 0x05);       /* [bp-0x0b] */
    uint16_t v0a = (uint16_t)(fp + 0x06);       /* [bp-0x0a] */
    uint16_t v08 = (uint16_t)(fp + 0x08);       /* [bp-8] */
    uint16_t v06 = (uint16_t)(fp + 0x0a);       /* [bp-6] */
    uint16_t v04 = (uint16_t)(fp + 0x0c);       /* [bp-4] */
    uint16_t v02 = (uint16_t)(fp + 0x0e);       /* [bp-2] */
    uint16_t si = rec;
    uint16_t di, bx;

    game_fread_far(file, (uint16_t)(si + 0x04));
    game_fread_far(file, (uint16_t)(si + 0x06));
    game_fread_far(file, (uint16_t)(si + 0x94));
    DGU16((uint16_t)(si + 0x08)) = DGU16((uint16_t)(si + 0x94));

    if (DG16(0x5474) >= 0x101)
        game_fread_far(file, (uint16_t)(si + 0x0a));

    game_fread_far(file, (uint16_t)(si + 0x90));
    DGU16((uint16_t)(si + 0x0c)) = DGU16((uint16_t)(si + 0x90));

    game_fread_far(file, (uint16_t)(si + 0x92));
    DGU16((uint16_t)(si + 0x12)) = DGU16((uint16_t)(si + 0x92));

    game_fread_far(file, (uint16_t)(si + 0x44));
    game_fread_far(file, (uint16_t)(si + 0x46));
    DGU16((uint16_t)(si + 0x42)) = DGU16((uint16_t)(si + 0x46));
    DGU16((uint16_t)(si + 0x40)) = DGU16((uint16_t)(si + 0x44));

    game_fread_far(file, (uint16_t)(si + 0x50));
    game_fread_far(file, (uint16_t)(si + 0x52));
    game_fread_far(file, (uint16_t)(si + 0x8c));
    game_fread_far(file, (uint16_t)(si + 0x8e));
    game_fread_far(file, (uint16_t)(si + 0x96));

    game_fread_far(file, v02);
    game_fread_byte(file, (uint16_t)(si + 0x56));
    game_fread_byte(file, (uint16_t)(si + 0x57));
    game_fread_far(file, (uint16_t)(si + 0x58));

    if (DG16(v02) != 0) {
        uint16_t rope = heap_calloc_far(1, 0x38);

        DGU16((uint16_t)(si + 0x54)) = rope;
        DGU16(v0e) = rope;
        DGU16((uint16_t)(DGU16(v0e) + 2)) = si;

        game_fread_far(file, v06);
        DGU16((uint16_t)(DGU16(v0e) + 4)) =
            (uint16_t)lookup_table_546c((int16_t)DGU16(v06));

        game_fread_far(file, v06);
        DGU16((uint16_t)(DGU16(v0e) + 6)) =
            (uint16_t)lookup_table_546c((int16_t)DGU16(v06));

        if (DGU16((uint16_t)(DGU16(v0e) + 4)) != 0)
            DGU16((uint16_t)(DGU16((uint16_t)(DGU16(v0e) + 4)) + 0x54)) =
                DGU16(v0e);

        if (DGU16((uint16_t)(DGU16(v0e) + 6)) != 0)
            DGU16((uint16_t)(DGU16((uint16_t)(DGU16(v0e) + 6)) + 0x54)) =
                DGU16(v0e);
    }

    for (DGU16(v0a) = 0; DG16(v0a) < 2; DGU16(v0a)++) {
        game_fread_far(file, v04);
        game_fread_byte(file,
                        (uint16_t)(si + 0x6a + 2 * DGU16(v0a)));
        game_fread_byte(file,
                        (uint16_t)(si + 0x6b + 2 * DGU16(v0a)));

        if (DG16(v04) == 0)
            continue;

        di = heap_calloc_far(1, 0x2c);
        DGU16((uint16_t)(si + 0x66 + 2 * DGU16(v0a))) = di;
        DGU16(DGU16((uint16_t)(si + 0x66 + 2 * DGU16(v0a)))) = si;

        game_fread_far(file, v06);
        DGU16((uint16_t)(di + 2)) =
            (uint16_t)lookup_table_546c((int16_t)DGU16(v06));
        DGU16((uint16_t)(di + 6)) = DGU16((uint16_t)(di + 2));

        game_fread_far(file, v06);
        DGU16((uint16_t)(di + 4)) =
            (uint16_t)lookup_table_546c((int16_t)DGU16(v06));
        DGU16((uint16_t)(di + 8)) = DGU16((uint16_t)(di + 4));

        game_fread_byte(file, (uint16_t)(di + 0x0a));
        DG8((uint16_t)(di + 0x0c)) = DG8((uint16_t)(di + 0x0a));
        game_fread_byte(file, (uint16_t)(di + 0x0b));
        DG8((uint16_t)(di + 0x0d)) = DG8((uint16_t)(di + 0x0b));

        if (DGU16((uint16_t)(di + 2)) != 0)
            DGU16((uint16_t)(DGU16((uint16_t)(di + 2))
                             + 0x66 + 2 * DG8((uint16_t)(di + 0x0a)))) = di;

        if (DGU16((uint16_t)(di + 4)) != 0)
            DGU16((uint16_t)(DGU16((uint16_t)(di + 4))
                             + 0x66 + 2 * DG8((uint16_t)(di + 0x0b)))) = di;
    }

    for (DGU16(v0a) = 0; DG16(v0a) < 2; DGU16(v0a)++) {
        game_fread_far(file, v06);
        DGU16((uint16_t)(si + 0x5a + 2 * (DGU16(v0a) + 2))) =
            (uint16_t)lookup_table_546c((int16_t)DGU16(v06));
        DGU16((uint16_t)(si + 0x5a + 2 * DGU16(v0a))) =
            DGU16((uint16_t)(si + 0x5a + 2 * (DGU16(v0a) + 2)));
    }

    if (DG16(0x5474) >= 0x101) {
        for (DGU16(v0a) = 4; DG16(v0a) < 6; DGU16(v0a)++) {
            game_fread_far(file, v06);
            DGU16((uint16_t)(si + 0x5a + 2 * DGU16(v0a))) =
                (uint16_t)lookup_table_546c((int16_t)DGU16(v06));
        }
    }

    if (DGU16((uint16_t)(si + 4)) == 7) {
        game_fread_far(file, v06);
        DGU16(v10) = (uint16_t)lookup_table_546c((int16_t)DGU16(v06));
        if (DGU16(v10) != 0)
            DGU16((uint16_t)(si + 0x68)) =
                DGU16((uint16_t)(DGU16(v10) + 0x66));
    }

    if (DG16(0x5474) <= 0x101) {
        game_fread_far(file, v08);
        if (DG16(v08) != 0) {
            for (DGU16(v0a) = 0; DG16(v0a) < DG16(v08); DGU16(v0a)++) {
                game_fread_byte(file, v0b);
                game_fread_byte(file, v0b);
            }
        }
    }

    bx = (uint16_t)((int16_t)DG16((uint16_t)(si + 4)) * 0x3a);
    DGU16((uint16_t)(si + 0x80)) = DGU16((uint16_t)(bx + 0x0ec4));

    if (DGU16((uint16_t)(si + 0x80)) != 0)
        DGU16((uint16_t)(si + 0x82)) =
            heap_calloc_far(DGU16((uint16_t)(si + 0x80)), 4);

    bx = (uint16_t)((int16_t)DG16((uint16_t)(si + 4)) * 0x3a);
    call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                    DGU16((uint16_t)(bx + 0x0ed2)), si);

    dg_leave(0x10);
}

/*
 * 0x1221b
 *
 * Read `n` things out of the file and put them on a list.
 *
 * DGROUP 0x5470 counts them, and each one's number is turned into its record by
 * `lookup_table_546c` before being read into - so the records were made in
 * advance by `alloc_part_table` and this only fills them. `insert_sorted` puts
 * each on the list the caller named, which is why the three lists this is
 * called for come out in the order the file's contents demand rather than the
 * order they were read.
 *
 * The list head is cleared first, both words of it.
 */
void read_list(uint16_t file, uint16_t head, int16_t n)
{
    int16_t di;

    DGU16((uint16_t)(head + 2)) = 0;
    DGU16(head) = 0;

    for (di = 0; di < n; di++) {
        uint16_t rec = (uint16_t)lookup_table_546c((int16_t)DGU16(0x5470));

        read_record_fields(file, rec);
        insert_sorted(rec, head);
        DGU16(0x5470)++;
    }
}

/*
 * 0x12269
 *
 * Read a .gkc file: the format the title screen, the credits and the game's
 * saved machines are all in.
 *
 * The first word must be **0xaced** or the whole thing is abandoned - and
 * abandoned quietly, by closing the file and answering with DGROUP 0x50d3
 * pointing at the empty list, not by saying anything.
 *
 * What follows depends on DGROUP 0x5472, which the caller sets: with it clear
 * the file is one of the intro animations and its play area and title are not
 * read; with it set they are, and so is a third list. That is one format
 * serving two purposes, and 0x5472 is how the reader is told which it is
 * looking at.
 *
 * Then three counts, room for that many parts in one go, and three lists read
 * into DGROUP 0x521b, 0x5179 and 0x50d7. The far table the records live in is
 * freed at the end - the records themselves are on the lists by then, and it
 * was only ever the scaffolding that got them there.
 */
uint16_t load_animation_into(uint16_t name)
{
    uint16_t fp = dg_enter(0x216);
    uint16_t buf = fp;                          /* [bp-0x216] */
    uint16_t n0 = (uint16_t)(fp + 0x214);       /* [bp-2] */
    uint16_t n1 = (uint16_t)(fp + 0x212);       /* [bp-4] */
    uint16_t n2 = (uint16_t)(fp + 0x210);       /* [bp-6] */
    uint16_t si;

    si = game_fopen(name, 0x2870);
    if (si == 0)
        goto out;

    stdio_setbuf_for(si, buf);

    game_fread_far(si, 0x5476);
    if (DGU16(0x5476) != 0xaced)
        goto close;

    game_fread_far(si, 0x5474);

    if (DGU16(0x5472) != 0) {
        game_fread_string(si, 0x4ecf);          /* the machine's name */
        game_fread_far(si, 0x50af);
        game_fread_far(si, 0x50b1);
    }

    game_fread_far(si, 0x50b3);
    game_fread_far(si, 0x50b5);

    recompute_kind_physics();

    if (DGU16(0x5472) != 0) {
        game_fread_far(si, 0x50b7);
        game_fread_far(si, 0x50b9);
    }

    game_fread_far(si, 0x50bb);

    game_fread_far(si, n0);
    game_fread_far(si, n1);
    game_fread_far(si, n2);

    DGU16(0x5470) = 0;

    alloc_part_table((int16_t)(DG16(n0) + DG16(n1) + DG16(n2)));

    read_list(si, 0x521b, DG16(n0));
    read_list(si, 0x5179, DG16(n1));

    if (DGU16(0x5472) != 0)
        read_list(si, 0x50d7, DG16(n2));

    dos_free_far(DGU16(0x546c), DGU16(0x546e));

close:
    game_fclose(si);

out:
    DGU16(0x50d3) = 0x50d7;

    dg_leave(0x216);
    return 0;
}

/*
 * 0x12915
 *
 * Load an animation file: build the part list first, clear DGROUP 0x5472, and
 * read it. The routine three bytes below does the same read while *preserving*
 * DGROUP 0x50d7 - this one lets the load replace it.
 */
uint16_t load_animation(uint16_t name)
{
    build_part_list();
    DGU16(0x5472) = 0;

    return load_animation_into(name);
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

