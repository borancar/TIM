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
    game_play();
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
    int16_t origin;                         /* the animation's left edge */
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
    origin = 0;

    if (DGU16(0x4e6b) == 0x8000) {
        which = (int16_t)0x8000;
        DGU16(0x4e6b) = 0x2000;
    } else {
        DGU16(0x4e6b) = 2;
        which = 2;
    }

    while ((uint16_t)which == 0x8000 || (uint16_t)which == 0x4000) {
        clear_flag_2d44_thunk();

        /*
         * The two animations are placed differently: the title screen sits
         * eight pixels left of the origin, the credits sixteen. Both branches
         * leave the value in AX and fall into the same six stores, which is why
         * it reads as one block with a number that is not constant.
         */
        if ((uint16_t)which == 0x8000) {
            load_animation(0x256c);                              /* "title.gkc"   */
            origin = -8;
        } else {
            load_animation(0x2576);                              /* "credits.gkc" */
            origin = -0x10;
        }

        DG16(0x4ea3) = origin;
        DG16(0x4e9f) = origin;
        DG16(0x4e9b) = origin;
        DG16(0x4ea1) = 0;
        DG16(0x4e9d) = 0;
        DG16(0x4e99) = 0;

        clear_machine();
        set_clip_full_screen();

        DGU16(0x38a8) = DGU16(0x38a2);
        vga_fill_colour = (uint8_t)DG8(0x52cb);
        vga_second_colour = (uint8_t)DG8(0x52cb);
        fill_enabled = 1;

        fill_rect(0, 0, 0x280, 0x190);

        step_and_draw_machine(1);
        draw_frame_corners(gkc);
        present_frame(1);

        DGU16(0x38a6) = DGU16(0x38a4);
        DGU16(0x38a8) = DGU16(0x38a2);
        copy_rect_around_cursor(0, 0, 0x280, 0x190);

        select_music((int16_t)((uint16_t)which == 0x8000 ? 0x3e9 : frame));

        running = 1;

        while (running != 0) {
            if (DGU16(0x52d3) != 0) DGU16(0x52d3) = 1;
            if (DGU16(0x52d1) != 0) DGU16(0x52d1) = 1;
            if (DGU16(0x52cf) != 0) DGU16(0x52cf) = 1;
            if (DGU16(0x52cd) != 0) DGU16(0x52cd) = 1;

            update_button_state();
            step_machine();
            mark_parts_in_dirty_rects();
            step_word_4e87();
            replay_shapes();

            step_and_draw_machine(0);
            draw_frame_corners(gkc);
            present_frame(1);

            if (DGU16(0x4ea7) == 0)
                set_palette_pointer(DGU16(0x52ed), DGU16(0x52ef));  /* tim.pal */

            if (DGU16(0x52d3) == 1) stop_music_or_effect(1);
            if (DGU16(0x52d1) == 1) stop_music_or_effect(2);
            if (DGU16(0x52cf) == 1) stop_music_or_effect(9);
            if (DGU16(0x52cd) == 1) stop_music_or_effect(0xc);

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
        reset_machine();

        for (si = 1; si <= 0x14; si++)
            stop_music_or_effect((uint16_t)si);

        free_all_lists();

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

    copy_protect_screen(gkc);

    DGU16(0x4e6b) = 2;

    set_palette_pointer(DGU16(0x52e1), DGU16(0x52e3));      /* black.pal */
    present_frame(1);

    free_bitmaps_thunk(gkc);

    stop_music_or_effect(0);
    show_cursor_again();

    DGU16(0x38a4) = 0xa190;
    DGU16(0x38a2) = 0xa8c0;
    DG16(0x3f7c) = 0x16f;

    vm_set_display_lines(0x1bf);
    vm_set_line_compare(0x16f);

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
 * **The copy-protection screen.** Thirty-two part icons in a grid of eight,
 * three empty slots, an OK button, and the line "Please select, in order, the
 * three parts listed on page N of the user's manual."
 *
 * The page is `(0x44ef & 0xf) + 1` - taken from the frame counter, so it is a
 * different page each time and the answer cannot be memorised. And the answer
 * is **a table**: the three parts wanted for page N are the Nth words of the
 * three arrays at DGROUP 0x24ea, 0x250a and 0x252a, sixteen pages each.
 *
 * The grid skips the parts that are not real: index `si` shows part `si`, or
 * `si + 1` past 0x13, with 0x1e becoming 0x23 and 0x20 becoming 0x24. Those
 * are the same holes `build_part_list` leaves - 0x14, 0x29 and 0x31 are never
 * offered - and the click at the bottom runs the identical remap on the cell it
 * lands in, so the two agree by construction rather than by a shared table.
 *
 * A click inside the grid writes the part into the next of the three slots and
 * wraps after the third, so a fourth click starts over. A click on the OK
 * button at 0x248,0x158 calls `sub_0e34a`. Tab - scancode 0x0f out of
 * `bios_read_key` - walks a highlight around the grid and onto the button.
 *
 * **The wait loop is entered on the wrong side, and exits at once.** After
 * `[bp-0x12]` is cleared the routine jumps to 0x0eddd, which *sets* it to 1,
 * and 0x0ede2 leaves when it is not zero. So the screen is drawn and the
 * routine returns without ever polling. That is not a reading of the listing:
 * the bytes at 0x0ec79 are `e9 61 01`, the next instruction is at 0x0ec7c, and
 * 0x0ec7c + 0x161 is 0x0eddd, which is a real instruction boundary. Checked
 * from the branch target, because a jump landing one byte off is how this
 * project has been wrong before.
 *
 * Two things follow, and the second is the one that matters. The loop body is
 * transcribed anyway, because it is there and has to be right if it is ever
 * reached. And the original **does not run this routine at all** while the
 * copy-protection screen is up: driven from a snapshot taken on that screen,
 * the guest executes zero addresses in 0x0ea39..0x0edf0 and sits in the driver
 * and the overlay instead. So whatever runs the screen the player sees, it is
 * not this. Recorded rather than explained away.
 */
uint16_t copy_protect_screen(uint16_t bitmaps)
{
    uint16_t fp      = dg_enter(0x74);
    uint16_t msg     = fp;              /* [bp-0x74], 0x50 bytes */
    uint16_t numbuf  = (uint16_t)(fp + 0x50);   /* [bp-0x24] */
    uint16_t answers = (uint16_t)(fp + 0x66);   /* [bp-0xe], three words */
    int16_t  page, done, slot, highlight, si;
    int16_t  x, y, part;

    vga_screen_height = 0x18f;

    for (si = 0; si < 3; si++)
        DG16((uint16_t)(answers + 2 * si)) = -1;

    highlight = -1;
    slot      = 0;
    page      = (int16_t)(DG16(0x44ef) & 0xf);

    set_clip_full_screen();
    DGU16(0x38a8) = DGU16(0x38a2);
    DG8(0x389d)   = DG8(0x52cb);
    DG8(0x389e)   = DG8(0x52cb);
    DG8(0x389c)   = 1;

    clear_flag_2d44_thunk();
    fill_rect(0, 0, 0x280, 0x190);
    restore_cursor_following();

    draw_frame_corners(bitmaps);

    draw_panel(0x30, 0x10, 0x220, 0xe0);         /* the panel */
    draw_panel(0xc0, 0x12c, 0x40, 0x30);         /* the three slots */
    draw_panel(0x120, 0x12c, 0x40, 0x30);
    draw_panel(0x180, 0x12c, 0x40, 0x30);
    draw_panel(0x248, 0x158, 0x20, 0x20);        /* the OK button */

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x24)), 0x24c, 0x15e, 0);
    restore_cursor_following();

    int_to_string((int16_t)(page + 1), numbuf, 10);
    string_copy(msg, 0x1c9e);   /* "Please select, in order, ... page " */
    string_concat(msg, numbuf);
    string_concat(msg, 0x1cd7); /* " of the user's manual." */
    draw_scroll_text(msg, 0x40, 0x106, 0x200);

    for (si = 0; si < 0x20; si++) {
        x    = (int16_t)(((si % 8) << 6) + 0x40);
        y    = (int16_t)((si / 8) * 0x30 + 0x20);
        part = si;
        if (part > 0x13)
            part++;
        if (part == 0x1e)
            part = 0x23;
        if (part == 0x20)
            part = 0x24;

        clear_flag_2d44_thunk();
        draw_bitmap_centred(DGU16((uint16_t)(DGU16(0x4ec7) + 2 * part)),
                            x, y, 0x40, 0x30);
        restore_cursor_following();
    }

    select_music((int16_t)(page + 0x3e9));
    present_frame(1);

    DGU16(0x38a6) = DGU16(0x38a4);
    DGU16(0x38a8) = DGU16(0x38a2);
    copy_rect_around_cursor(0, 0, 0x280, 0x190);
    set_palette_pointer(DGU16(0x52ed), DGU16(0x52ef));
    show_cursor_again();

    done = 0;
    goto check;                 /* 0x0ec79, and it lands past the test */

    for (;;) {
        update_button_state();

        DG8(0x52f1) = (uint8_t)(bios_read_key() >> 8);
        if (DG8(0x52f1) == 0x0f) {          /* Tab walks the highlight */
            highlight++;
            if (highlight == 0x21)
                highlight = 0;
            if (highlight == 0x20)
                move_pointer_to(0x268, 0x188);    /* the button */
            else
                move_pointer_to((uint16_t)(((highlight % 8) << 6) + 0x50),
                          (uint16_t)((highlight / 8) * 0x30 + 0x30));
        }

        select_cursor((DG16(0x5784) >= 0x248 && DG16(0x5782) >= 0x158)
                      ? 0x15 : 0);

        if (DG16(0x5774) == 2) {            /* the frame of a click */
            if (DG16(0x5784) >= 0x40 && DG16(0x5784) < 0x240
                && DG16(0x5782) >= 0x20 && DG16(0x5782) < 0xe0) {
                part = (int16_t)((DG16(0x5784) - 0x40) / 0x40
                                 + ((DG16(0x5782) - 0x20) / 0x30) * 8);
                if (part > 0x13)
                    part++;
                if (part == 0x1e)
                    part = 0x23;
                if (part == 0x20)
                    part = 0x24;

                DG16((uint16_t)(answers + 2 * slot)) = part;
                sub_0edf1(DGU16((uint16_t)(DGU16(0x4ec7) + 2 * part)),
                          (uint16_t)slot);
                slot++;
                if (slot == 3)
                    slot = 0;
            }

            if (DG16(0x5784) >= 0x248 && DG16(0x5782) >= 0x158)
                sub_0e34a(1);
        }

        present_frame(1);

        if (DG16((uint16_t)(0x24ea + 2 * page)) == DG16(answers)
            && DG16((uint16_t)(0x250a + 2 * page)) == DG16((uint16_t)(answers + 2))
            && DG16((uint16_t)(0x252a + 2 * page)) == DG16((uint16_t)(answers + 4)))
check:
            done = 1;

        if (done != 0)
            break;
    }

    dg_leave(0x74);
    return 0;
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
 * Draw the four corner pieces of the intro's frame, from the four bitmaps the
 * record holds: top left at the origin, top right at 0x262, bottom left at
 * 0x175, bottom right at both. The positions are constants in the code, so the
 * frame is the same size whatever is inside it.
 */
void draw_frame_corners(uint16_t rec)
{
    clear_flag_2d44_thunk();

    draw_bitmap(DGU16(rec), 0, 0, 0);
    draw_bitmap(DGU16((uint16_t)(rec + 2)), 0x262, 0, 0);
    draw_bitmap(DGU16((uint16_t)(rec + 4)), 0, 0x175, 0);
    draw_bitmap(DGU16((uint16_t)(rec + 6)), 0x262, 0x175, 0);

    restore_cursor_following();
}

/*
 * 0x0eed5
 *
 * **The game.** `game_main`'s third call, and everything after the intro and the
 * copy protection is inside it: set up, run rounds until something says stop,
 * take it all down.
 *
 * The loop is a `while` and not a `do`, and that matters - the jump at 0x0eedd
 * goes to the test, so `game_setup` has to leave DGROUP 0x4ebf non-zero or the
 * game ends before a single round runs.
 *
 * A round is `game_round`, and what happens afterwards depends on the state at
 * 0x4e6b. When it is 1 the loop is stopped by clearing 0x4ebf; otherwise the
 * count at 0x4ebd goes up, and if it has passed the best at 0x4eb7 the best is
 * caught up and 0x12bed is called - which is the shape of a high score being
 * written out.
 *
 * The counter is compared with `jle`, so the record is only rewritten when it
 * is genuinely beaten and not on a tie.
 */
void game_play(void)
{
    game_setup();

    while (DG16(0x4ebf) != 0) {
        game_round();

        if (DG16(0x4e6b) == 1) {
            DG16(0x4ebf) = 0;
        } else {
            DG16(0x4ebd) = (int16_t)(DG16(0x4ebd) + 1);
            if (DG16(0x4ebd) > DG16(0x4eb7)) {
                DG16(0x4eb7) = DG16(0x4ebd);
                sub_12bed();
            }
        }
    }

    free_two_bitmap_lists();
}

/*
 * 0x0ef19
 *
 * Set the game up: draw the status bar across the top of the screen, load the
 * two bitmap sets the game keeps for the whole of its run, and start the
 * counters.
 *
 * The bar is three pieces of `score1.bmp` laid at x = 3, 0x107 and 0x1bb on a
 * band cleared to colour 0 - `fill_rect(0, 0, 0x280, 0x50)`, the top eighty
 * rows - and drawn straight into 0xa000 rather than into whichever page is
 * being built, the same way the odometer at 0x026e8 does. `score1.bmp`'s list
 * is **freed immediately afterwards**: it is wanted once, for this, and the
 * three pictures are already on the screen.
 *
 * What is kept is `gp_menu.bmp` at DGROUP 0x4ec9 and `score2.bmp` at 0x4ecd -
 * and 0x4ecd is the list `draw_odometer_digit` takes its two digit strips
 * from, which is what makes the rolling counters possible from here on.
 *
 * Then the state: the 32-bit counter at 0x4ead/0x4eaf to zero, the round count
 * at 0x4ebd to **1** rather than 0, and 0x4ebf to 1 - which is the one that
 * matters, because `game_play` tests it before running anything and a zero
 * here would end the game before it started.
 */
void game_setup(void)
{
    uint16_t bar;

    clear_flag_2d44_thunk();
    bar = load_bitmaps(0x25e8);                 /* "score1.bmp" */

    DGU16(0x38a8) = 0xa000;
    DG8(0x389d) = 0;
    DG8(0x389e) = 0;
    DG8(0x389c) = 1;

    fill_rect(0, 0, 0x280, 0x50);

    draw_bitmap(DGU16(bar), 3, 0, 0);
    draw_bitmap(DGU16((uint16_t)(bar + 2)), 0x107, 0, 0);
    draw_bitmap(DGU16((uint16_t)(bar + 4)), 0x1bb, 0, 0);

    free_bitmaps_thunk(bar);

    clear_flag_2d44_thunk();
    DGU16(0x4ec9) = load_bitmaps(0x25f3);       /* "gp_menu.bmp" */
    DGU16(0x4ecd) = load_bitmaps(0x25ff);       /* "score2.bmp"  */

    DGU16(0x4eaf) = 0;
    DGU16(0x4ead) = 0;
    DGU16(0x4ebd) = 1;
    DGU16(0x4ebf) = 1;
    DGU16(0x4e67) = 0;
}

/*
 * 0x0f04b
 *
 * Start a round: put the machine's six origins back to -8, clear the counters
 * and the input, and either rebuild the parts list or load a level.
 *
 * The six words at DGROUP 0x4e99 through 0x4ea3 are set to 0xfff8 - three
 * pairs, all -8 - and so are 0x50b7 and 0x50b9. That is the same -8 origin
 * `build_part_list` writes and which its comment already flags as reading like
 * a mistake and not being one.
 *
 * The fork at 0x4e67 is which kind of round this is: non-zero rebuilds the
 * parts list and resets the machine, which is the free-play shape; zero loads
 * the level whose number is the round count at 0x4ebd, which for the first
 * round is 1 - and that is where L1.LEV is read.
 *
 * Then `start_counters` - the odometer at 0x024fa, transcribed long before
 * anything could reach it and marked unverified for exactly that reason. This
 * is its caller.
 *
 * The state at 0x4e6b is left at 2, which is what sends `game_round`'s
 * dispatch to 0x10f03 on the first pass.
 */
void round_setup(void)
{
    DG16(0x4ea1) = -8;
    DG16(0x4ea3) = -8;
    DG16(0x4e9d) = -8;
    DG16(0x4e9f) = -8;
    DG16(0x4e99) = -8;
    DG16(0x4e9b) = -8;
    DGU16(0x4ebb) = 0;

    heap_check_or_hang();

    if (DGU16(0x4e67) != 0) {
        build_part_list();
        reset_machine();
    } else {
        load_level(DGU16(0x4ebd));
    }

    DG16(0x50b9) = -8;
    DG16(0x50b7) = -8;

    start_counters();
    reset_input_state();

    DGU16(0x4e6b) = 2;
}

/*
 * 0x0eff5
 *
 * **One round**, as a state machine on DGROUP 0x4e6b.
 *
 * After `round_setup` the state is 2, and each pass through the loop checks
 * the heap and then dispatches on it:
 *
 *   2       0x10f03 - and 0x4e6b being left at 2 by the setup is what makes
 *           this the first screen of a round
 *   0x2000  0x012ab
 *   other   0x0f8c2
 *
 * The two that end the round are 0x200 and 1, tested at the bottom, so a
 * screen leaves by writing one of those into 0x4e6b rather than by returning
 * anything. And 0x200 alone gets 0x02710 called on the way out, which is the
 * one asymmetry in it.
 *
 * A `while` again rather than a `do`: the entry jump at 0x0effd goes to the
 * test. With the state at 2 the test passes, so the loop always runs at least
 * once in practice - but it is written as a test-first loop and is transcribed
 * as one.
 */
void game_round(void)
{
    round_setup();

    while (DGU16(0x4e6b) != 0x200 && DGU16(0x4e6b) != 1) {
        heap_check_or_hang();

        if (DGU16(0x4e6b) == 2)
            game_screen();
        else if (DGU16(0x4e6b) == 0x2000)
            sub_012ab();
        else
            sub_0f8c2();
    }

    if (DGU16(0x4e6b) == 0x200)
        sub_02710();

    round_teardown();
}

/*
 * 0x12863
 *
 * Load a level by number: build its name and hand it to `read_level`.
 *
 * The name is assembled a piece at a time out of DGROUP - "l" at 0x2876, the
 * number in decimal, ".lev" at 0x2878 - into a 0x16-byte buffer on the stack.
 * `round_setup` passes the round count at 0x4ebd, so the first round asks for
 * "l1.lev", which is the name the resource archive holds.
 *
 * The flag at 0x5472 is set to 1 before the read and is not cleared here.
 */
void load_level(uint16_t number)
{
    uint16_t fp     = dg_enter(0x16);
    uint16_t name   = fp;
    uint16_t digits = (uint16_t)(fp + 0xe);

    string_copy(name, 0x2876);
    int_to_string((int16_t)number, digits, 10);
    string_concat(name, digits);
    string_concat(name, 0x2878);

    DGU16(0x5472) = 1;
    read_level(name);

    dg_leave(0x16);
}



/*
 * 0x117ed
 *
 * **The title bar and the hint box** - the two pieces of text across the top
 * of the game screen, and the first thing `paint_game_screen` draws over the
 * cleared play area.
 *
 * The title is built in a 0x80-byte buffer and depends on the mode at DGROUP
 * 0x4e67. Free play gets "FREEFORM MODE" and nothing else. A level gets
 * "PUZZLE ", the round number from 0x4ebd, the separator at 0x2837, and then
 * the level's own title from **0x4ecf** - which `read_level` filled in from the
 * file. So "PUZZLE 1: TUTORIAL: PUT THE BALL IN THE HOOP" is three pieces from
 * three places, and only the middle one is a number.
 *
 * Then the drawing: a bar at (0x20, 0x20) 0x220 by 0x158 through 0x14de:0x000c,
 * a filled area at (0x110, 0x48) in the colour at 0x52cb, the title centred on
 * a scroll at (0x3c, 0x27) 0x1bc wide, and a panel at (0x110, 0xff).
 *
 * The hint below it comes from the same fork: free play gets the fixed string
 * at 0x22c0 about creating any machine you wish, and a level gets **0x4f1f**,
 * the hint `read_level` read out of the file - which for level one is "Make the
 * basketball go through the hoop." Both are drawn into the same box at
 * (0x114, 0x104) 0xf8 by 0x44, so the two paths differ only in the string.
 */
void paint_panel_frame(void)
{
    uint16_t fp     = dg_enter(0x80);
    uint16_t title  = fp;
    uint16_t digits = (uint16_t)(fp + 0x78);

    if (DGU16(0x4e67) != 0) {
        string_copy(title, 0x21d4);             /* "FREEFORM MODE" */
    } else {
        string_copy(title, 0x21e2);             /* "PUZZLE " */
        int_to_string(DG16(0x4ebd), digits, 10);
        string_concat(title, digits);
        string_concat(title, 0x2837);
        string_concat(title, 0x4ecf);           /* the level's own title */
    }

    set_clip_play_area();
    DGU16(0x38a8) = DGU16(0x38a2);

    draw_title_bar(0x20, 0x20, 0x220, 0x158, 1);
    fill_panel_area(0x110, 0x48, 0x100, 0xa0, DGU16(0x52cb));

    draw_scroll_text(title, 0x3c, 0x27, 0x1bc);
    draw_panel(0x110, 0xff, 0x100, 0x4c);

    if (DGU16(0x4e67) != 0)
        draw_wrapped_text(0x22c0, 0x114, 0x104, 0xf8, 0x44);
    else
        draw_wrapped_text(0x4f1f, 0x114, 0x104, 0xf8, 0x44);

    paint_panel_frame_rest();

    dg_leave(0x80);
}

/*
 * 0x14dec
 *
 * **The frame the title bar sits in**: a shadow, a tiled interior, and a
 * border of edge and corner pieces from the set at DGROUP 0x4ecb.
 *
 * The rectangle arrives as **two corners and not a size**, which is worth
 * saying because the call passes 0x220 and 0x158 and those read as a width and
 * a height: every use of them here is a subtraction, `x2 - x1` and `y2 - y1`.
 *
 * `filled` gates the first part - a filled rectangle offset down and left of
 * the frame, and two pieces at +0x4a and +0x4c - which is the drop shadow, so
 * a caller can have the frame without it.
 *
 * Then the interior. The clip box is set to the four corners and the tile at
 * +0x54 is laid in steps of 0x80 across and 0x40 down, so one tile covers any
 * size. The clip then goes back to the whole screen or to the play area
 * depending on whether the state at 0x4e6b is 0x8000 - the same fork
 * `draw_panel` makes, and it has to happen before the border is drawn or the
 * border would be clipped away by its own frame.
 *
 * The border is four runs of 8 pixels - top and bottom together in one loop
 * across x, left and right together in one loop down y - and then four
 * corners, each placed by an offset from its own corner rather than from the
 * origin. Nine pieces in all: +0x20 to +0x26 for the runs, +0x18 to +0x1e for
 * the corners.
 */
void draw_title_bar(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t filled)
{
    uint16_t set = DGU16(0x4ecb);
    int16_t  x, y;

    DGU16(0x38a8) = DGU16(0x38a2);
    clip_enabled = 0;
    fill_enabled = 1;
    vga_fill_colour   = 0;
    vga_second_colour = 0;

    clear_flag_2d44_thunk();

    if (filled != 0) {
        fill_rect((int16_t)(x1 - 0x0c), (int16_t)(y1 + 0x0c),
                  (int16_t)(x2 - x1), (int16_t)(y2 - y1));
        draw_bitmap(DGU16((uint16_t)(set + 0x4a)),
                    (int16_t)(x1 - 0x0f), (int16_t)(y1 + 7), 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x4c)),
                    (int16_t)(x1 - 0x0f), (int16_t)(y2 - 9), 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x4e)),
                    (int16_t)(x2 - 0x20), (int16_t)(y2 - 9), 0);
    }

    clip_left    = x1;
    clip_right   = x2;
    clip_top     = y1;
    clip_bottom  = y2;
    clip_enabled = 1;

    for (y = y1; y < y2; y = (int16_t)(y + 0x40))
        for (x = x1; x < x2; x = (int16_t)(x + 0x80))
            draw_bitmap(DGU16((uint16_t)(set + 0x54)), x, y, 0);

    if (DGU16(0x4e6b) == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    clip_enabled = 0;

    for (x = x1; x < x2; x = (int16_t)(x + 8)) {
        draw_bitmap(DGU16((uint16_t)(set + 0x24)), x, (int16_t)(y1 - 4), 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x26)), x, y2, 0);
    }

    for (y = y1; y < y2; y = (int16_t)(y + 8)) {
        draw_bitmap(DGU16((uint16_t)(set + 0x20)), (int16_t)(x1 - 4), y, 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x22)), x2, y, 0);
    }

    draw_bitmap(DGU16((uint16_t)(set + 0x18)),
                (int16_t)(x1 - 7), (int16_t)(y1 - 7), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x1a)),
                (int16_t)(x2 - 0x11), (int16_t)(y1 - 7), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x1c)),
                (int16_t)(x1 - 7), (int16_t)(y2 - 0x11), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x1e)),
                (int16_t)(x2 - 0x11), (int16_t)(y2 - 0x11), 0);
}

/*
 * 0x15523
 *
 * **A filled, framed area** of the panel: a rectangle in a given colour with
 * the same nine-piece border around it that `draw_title_bar` uses - four runs
 * of 8 pixels and four corners, from the set at DGROUP 0x4ecb.
 *
 * Unlike `draw_title_bar` this one takes a **width and a height** and works
 * out the far corner itself, into two locals, on the way in. The two routines
 * draw the same kind of frame and disagree about how to be told where it goes,
 * which is worth knowing before reading either from memory of the other.
 *
 * The border pieces are a different set from the title bar's: +0x34 and +0x36
 * for the top and bottom runs, +0x30 and +0x32 for the sides, +0x28 to +0x2e
 * for the corners. All four corners sit 8 pixels out except the bottom-left,
 * which is **5** - `0xfffb` and not `0xfff8`, once, and it is not a
 * misreading: the byte is `fb`.
 *
 * The colour is passed in and written to both 0x389d and 0x389e before the
 * fill, so the interior and whatever else reads the second colour agree.
 */
void fill_panel_area(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t colour)
{
    uint16_t set = DGU16(0x4ecb);
    int16_t  x2  = (int16_t)(x + w);
    int16_t  y2  = (int16_t)(y + h);
    int16_t  n;

    clear_flag_2d44_thunk();
    DGU16(0x38a8) = DGU16(0x38a2);

    DG8(0x389d) = (uint8_t)colour;
    DG8(0x389e) = (uint8_t)colour;

    fill_rect(x, y, w, h);

    for (n = x; n < x2; n = (int16_t)(n + 8)) {
        draw_bitmap(DGU16((uint16_t)(set + 0x34)), n, (int16_t)(y - 8), 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x36)), n, y2, 0);
    }

    for (n = y; n < y2; n = (int16_t)(n + 8)) {
        draw_bitmap(DGU16((uint16_t)(set + 0x30)), (int16_t)(x - 8), n, 0);
        draw_bitmap(DGU16((uint16_t)(set + 0x32)), x2, n, 0);
    }

    draw_bitmap(DGU16((uint16_t)(set + 0x28)),
                (int16_t)(x - 8), (int16_t)(y - 8), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x2a)),
                (int16_t)(x2 - 8), (int16_t)(y - 8), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x2c)),
                (int16_t)(x - 8), (int16_t)(y2 - 5), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x2e)),
                (int16_t)(x2 - 8), (int16_t)(y2 - 8), 0);
}

/*
 * 0x13dc7
 *
 * **Draw a string wrapped into a box**, centred both ways, with a shadow.
 *
 * `wrap_text_to_box` does the wrapping and leaves its results in DGROUP: a
 * list of line pointers from 0x56a6, how many at 0x56a4, and the block's
 * measured height and width at 0x56a0 and 0x56a2. This routine only places and
 * draws them.
 *
 * The centring uses the *measured* extents, not the box: `(w - 0x56a2 - 1) / 2`
 * and `(h - 0x56a0 - 1) / 2`, the minus one making an odd remainder fall left
 * and up rather than right and down. The clip box is then set to the box as
 * placed, so a line the wrapper could not fit is cut rather than drawn over
 * the panel.
 *
 * **A line's end is the next line's start, less one.** The table holds only
 * starts, so each line is bounded by looking ahead - and the trailing spaces
 * are walked back over before drawing, then a NUL is written *into the
 * caller's string* to terminate it and the displaced byte is put back
 * afterwards. The string is modified and restored, which is why this cannot be
 * handed a string in read-only memory.
 *
 * Each line is drawn twice, colour 0xf one pixel left and one down and then
 * colour 5 at the true place - the same shadow the parts bin's numbers use.
 *
 * The loop ends on a null pointer, on a line that starts with a NUL, or when
 * the count runs out, and the count is tested **before** it is decremented, so
 * a count of one draws one line.
 */
void draw_wrapped_text(uint16_t str, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t line_height;
    uint16_t entry;
    int16_t  left, top, left_at;

    DG8(0x3892) = 1;                        /* transparent */
    line_height = font_line_height(0);

    wrap_text_to_box(str, w, h, line_height);

    left = (int16_t)(x + (w - DG16(0x56a2) - 1) / 2);
    top  = (int16_t)(y + (h - DG16(0x56a0) - 1) / 2 + 1);

    clip_left   = left;
    clip_right  = (int16_t)(left + w);
    clip_top    = top;
    clip_bottom = (int16_t)(top + h);

    entry   = 0x56a6;
    left_at = DG16(0x56a4);

    while (DGU16(entry) != 0 && DG8(DGU16(entry)) != 0 && left_at-- != 0) {
        uint16_t start = DGU16(entry);
        uint16_t end   = (uint16_t)(DGU16((uint16_t)(entry + 2)) - 1);
        uint8_t  saved;

        while (end > start && DG8(end) <= ' ')
            end--;
        end++;

        saved = DG8(end);
        DG8(end) = 0;

        clear_flag_2d44_thunk();

        DG8(0x3890) = 0x0f;
        draw_string(start, (int16_t)(left - 1), (int16_t)(top + 1));

        DG8(0x3890) = 5;
        draw_string(start, left, top);

        restore_cursor_following();

        DG8(end) = saved;
        entry = (uint16_t)(entry + 2);
        top = (int16_t)(top + line_height);
    }

    set_clip_full_screen();
}

/*
 * 0x13ed2
 *
 * **Break a string into lines that fit a box.** The line starts go into the
 * table from DGROUP 0x56a6, how many at 0x56a4, and the block's measured
 * height and width at 0x56a0 and 0x56a2 - which `draw_wrapped_text` then uses
 * to centre it.
 *
 * The height is capped at **seven lines** before anything else: `h` is reduced
 * to `7 * line_height` if it is larger, so a tall box does not make a tall
 * block. Seven is a constant in the code, not a table size.
 *
 * The measuring is by *word*, through `measure_word`, which answers the word's
 * width and its length. A word that does not fit starts a new line - and the
 * test is `width + word > box` **or** nothing has been placed on this line yet
 * and the block is not empty, so a single word wider than the box still gets a
 * line to itself rather than looping.
 *
 * A carriage return, 0x0d, forces a line break and the next line starts *after*
 * it. A space adds the width of a space - measured once at the top from a
 * two-byte string - and is otherwise skipped. Any other character at or below
 * a space ends the scan.
 *
 * The width recorded at 0x56a2 is the widest line, clamped to the box.
 *
 * **The last line is counted only if it has something on it**: after the loop,
 * a run width of zero with at least one line already recorded takes one back
 * off the count; otherwise the height gains one more line. Then the entry past
 * the last is set to the point the scan stopped at, which is what makes
 * `draw_wrapped_text`'s "end is the next start, less one" work for the final
 * line as well.
 */
void wrap_text_to_box(uint16_t str, int16_t w, int16_t h, uint16_t line_height)
{
    uint16_t fp     = dg_enter(0x0c);
    uint16_t space  = fp;            /* [bp-0xc], a two-byte " " */
    uint16_t o_len  = (uint16_t)(fp + 2);   /* [bp-0xa] */
    uint16_t o_wide = (uint16_t)(fp + 8);   /* [bp-4]   */
    uint16_t at     = str;
    int16_t  used   = 0;         /* height used so far */
    int16_t  run    = 0;         /* width on the current line */
    int16_t  space_w;
    int16_t  cap    = (int16_t)(line_height * 7);

    if (h > cap)
        h = cap;

    DGU16(0x56a4) = 0;
    DG16(0x56a0)  = 0;
    DG16(0x56a2)  = 0;

    if (DG8(at) != 0) {
        DGU16((uint16_t)(0x56a6 + 2 * DGU16(0x56a4))) = at;
        DGU16(0x56a4)++;
    }

    DG8(space)     = ' ';
    DG8(space + 1) = 0;
    space_w = (int16_t)text_width_thunk(space);

    while (DG8(at) != 0 && (int16_t)(used + line_height) < h) {
        int16_t word_w, word_len;

        measure_word(at, o_wide, o_len);
        word_w   = DG16(o_wide);
        word_len = DG16(o_len);

        if ((run != 0 || used == 0) && (int16_t)(run + word_w) >= w) {
            run  = 0;
            used = (int16_t)(used + line_height);
            DGU16((uint16_t)(0x56a6 + 2 * DGU16(0x56a4))) = at;
            DGU16(0x56a4)++;
            if ((int16_t)(used + line_height) >= h)
                break;
        }

        at  = (uint16_t)(at + word_len);
        run = (int16_t)(run + word_w);
        if (run > DG16(0x56a2))
            DG16(0x56a2) = run;
        if (DG16(0x56a2) > w)
            DG16(0x56a2) = w;

        while (DG8(at) != 0 && DG8(at) <= ' '
               && (int16_t)(used + line_height) < h) {
            if (DG8(at) == 0x0d) {
                run  = 0;
                used = (int16_t)(used + line_height);
                DGU16((uint16_t)(0x56a6 + 2 * DGU16(0x56a4))) =
                    (uint16_t)(at + 1);
                DGU16(0x56a4)++;
            } else if (DG8(at) == ' ') {
                run = (int16_t)(run + space_w);
            }
            at++;
        }
    }

    DG16(0x56a0) = used;

    if (run == 0 && DGU16(0x56a4) != 0)
        DGU16(0x56a4)--;
    else
        DG16(0x56a0) = (int16_t)(DG16(0x56a0) + line_height);

    DGU16((uint16_t)(0x56a6 + 2 * DGU16(0x56a4))) = at;

    dg_leave(0x0c);
}

/*
 * 0x1401d
 *
 * Measure one word: how wide it is and how long, answered through the two near
 * pointers it is given.
 *
 * A word runs to the first character **at or below a space** - so a space, a
 * carriage return and a NUL all end it, and `wrap_text_to_box` then decides
 * which of those it was.
 *
 * The width comes from `text_width`, and to get it the routine writes a NUL
 * over the terminator, measures, and puts the displaced byte back - the same
 * trick `draw_wrapped_text` uses on the same string, for the same reason:
 * `text_width` stops at a NUL and there is nowhere else to put one.
 *
 * The length is counted separately as the walk goes rather than taken from the
 * pointer difference.
 */
void measure_word(uint16_t str, uint16_t out_width, uint16_t out_length)
{
    uint16_t at  = str;
    int16_t  len = 0;
    uint8_t  saved;

    while (DG8(at) > ' ') {
        at++;
        len++;
    }

    saved   = DG8(at);
    DG8(at) = 0;

    DG16(out_width)  = (int16_t)text_width(str);
    DG16(out_length) = len;

    DG8(at) = saved;
}

/*
 * 0x1175c
 *
 * **Draw the machine's parts into the play area**, which is the last thing the
 * title bar's painter does and the thing that puts the level's contents on the
 * screen.
 *
 * The scale comes from the level's own origin: the *larger* of 0x50b7 and
 * 0x50b9 plus 0x230, divided into 4 as a 32-bit division. Both are -8 for a
 * fresh level, so the divisor is 0x228 and the result is 0 - but the code
 * takes the larger and divides, and a level with a different origin would get
 * a different answer.
 *
 * Then every part in the buckets is linked in and drawn: `pick_by_flag` with
 * 0x3000 answers the first, `pick_for_record` with 0x1000 walks on from it,
 * and each is passed to `link_record_into_buckets` on the way. The loop ends
 * when the walk answers zero - and it is a `while` whose test is the *result*
 * of the walk, so a machine with no parts draws nothing and does not fault.
 *
 * `draw_machine` is then given the scale and 0x200, and the clip is put back
 * to the play area.
 *
 * **The scale is 0x40000 divided by the extent**, which is 1024 units per
 * pixel over a 256-pixel panel - not the extent divided by 4. The two long
 * arguments at 0x1179a are pushed the way Borland pushes a long, high word
 * first, so `mov ax, 4 / xor dx, dx / push ax / push dx` puts 0x0004_0000 on
 * the stack and it is the *dividend*. Reading it as a divisor of 4 gave 138
 * where the original gives 474, and a machine drawn at a third of its size
 * scaled every part's position off the panel - which is how it was caught: the
 * blitter's row buffer overran into DGROUP 0x124 and the part walk there never
 * terminated.
 *
 * The two locals stepped by two - 0x100 and 0xa0 becoming 0x102 and 0xa2 - are
 * computed and never read. Transcribed as the dead stores they are.
 */
void paint_panel_frame_rest(void)
{
    int16_t  extent;
    int16_t  scale;
    uint16_t rec;

    clip_enabled = 1;
    set_clip_for_mode();

    extent = (DG16(0x50b7) > DG16(0x50b9)) ? DG16(0x50b7) : DG16(0x50b9);
    extent = (int16_t)(extent + 0x230);

    scale = (int16_t)long_divide(0x40000, (int32_t)extent);

    DGU16(0x38a8) = DGU16(0x38a2);

    rec = (uint16_t)pick_by_flag(0x3000);
    while (rec != 0) {
        link_record_into_buckets(rec);
        rec = (uint16_t)pick_for_record(rec, 0x1000);
    }

    draw_machine(scale, 0x200);

    set_clip_play_area();
}

/*
 * 0x1190d
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x20 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0x3a,0x5b.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_a(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x20)),
                0x3a, 0x5b, 0);
    restore_cursor_following();
}

/*
 * 0x11943
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x24 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0xd8,0x60.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_b(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x24)),
                0xd8, 0x60, 0);
    restore_cursor_following();
}

/*
 * 0x11979
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x3e / 2 + frame]` out of the list at DGROUP 0x52f4, at 0xbc,0x5c.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_c(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x3e)),
                0xbc, 0x5c, 0);
    restore_cursor_following();
}

/*
 * 0x119af
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x52 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0x6d,0x85.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_d(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x52)),
                0x6d, 0x85, 0);
    restore_cursor_following();
}

/*
 * 0x119e5
 *
 * Paint one of the free-play panel's pairs: bitmap `list[0x42 / 2 + frame]`
 * at 0x96,0x8c and then `list[0x3a / 2 + frame]` at 0xa6,0x8b, both
 * out of the list at DGROUP 0x52f4.
 *
 * Two bitmaps between one `clear_flag_2d44_thunk` and one
 * `restore_cursor_following`, not two of each - the cursor is lifted once and
 * put back once, so the second bitmap cannot land on a restored cursor.
 */
void paint_panel_free_a(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x42)),
                0x96, 0x8c, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x3a)),
                0xa6, 0x8b, 0);
    restore_cursor_following();
}

/*
 * 0x11a3f
 *
 * Paint one of the free-play panel's pairs: bitmap `list[0x46 / 2 + frame]`
 * at 0xc8,0x8c and then `list[0x3a / 2 + frame]` at 0xd8,0x8b, both
 * out of the list at DGROUP 0x52f4.
 *
 * Two bitmaps between one `clear_flag_2d44_thunk` and one
 * `restore_cursor_following`, not two of each - the cursor is lifted once and
 * put back once, so the second bitmap cannot land on a restored cursor.
 */
void paint_panel_free_b(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x46)),
                0xc8, 0x8c, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x3a)),
                0xd8, 0x8b, 0);
    restore_cursor_following();
}

/*
 * 0x11a99
 *
 * Paint the level indicator: bitmap `list[0x36 / 2 + frame]` out of the
 * list at DGROUP 0x52f4, at 0x39,0x86. The same six instructions as
 * `paint_panel_a`; see its comment for the shape.
 */
void paint_panel_level(uint16_t frame)
{
    DGU16(0x38a8) = DGU16(0x38a2);

    clear_flag_2d44_thunk();
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * frame + 0x36)),
                0x39, 0x86, 0);
    restore_cursor_following();
}

/*
 * 0x11acf
 *
 * The panel's tiled background and the row of indicators over it, all out of
 * the bitmap list at DGROUP 0x52f4.
 *
 * The background is one bitmap - `list[0x56 / 2]` - stamped on an eight-pixel
 * grid from x 0x84 to 0xb4 and y 0x5f to 0x77. The two bounds are tested
 * differently: `cmp si, 0xb4 / jl` stops before 0xb4 and `cmp di, 0x77 / jle`
 * includes 0x77, so the grid is six columns by four rows and not five by four.
 *
 * Two of the indicators depend on what the round is: DGROUP 0x4e6b holding
 * 0x4000 picks entry 0x26 over 0x25, and 0x2000 picks 0x28 over 0x27.
 *
 * The last loop draws one bitmap per part in the level, `list[0x28 / 2 + si]`,
 * at the x in the word table at DGROUP 0x2816 - which is indexed from 1, so
 * its first word is not an x - and at a y that starts at 0x69 and steps *down*
 * by two each time, so the row leans.
 */
void paint_panel_e(void)
{
    int16_t left, right, y;
    int16_t si, di;

    left  = (DGU16(0x4e6b) == 0x4000) ? 0x26 : 0x25;
    right = (DGU16(0x4e6b) == 0x2000) ? 0x28 : 0x27;

    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    for (si = 0x84; si < 0xb4; si = (int16_t)(si + 8))
        for (di = 0x5f; di <= 0x77; di = (int16_t)(di + 8))
            draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x56)), si, di, 0);

    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * left)),  0x58, 0x5d, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * right)), 0x58, 0x6f, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x28)),      0x6e, 0x60, 0);

    y = 0x69;
    for (si = 1; si <= DG16(0x4ec1); si++) {
        draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 2 * si + 0x28)),
                    DG16((uint16_t)(0x2816 + 2 * si)), y, 0);
        y = (int16_t)(y - 2);
    }

    restore_cursor_following();
}

/*
 * 0x11bd6
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b5.
 *
 * The knob's x is `0x50b5 * 0xa0 / 0x200 + 0x3d`, worked out as a long -
 * `mul16x16` then `long_divide` - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x200
 * against 0x80, so they are not the same slider at two positions.
 */
void paint_panel_f(void)
{
    int16_t at;

    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0xe)), 0x41, 0xc8, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x12)), 0x3d, 0xe5, 0);

    at = (int16_t)long_divide(
             (int32_t)mul16x16(DG16(0x50b5), 0xa0), 0x200);

    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x0c)),
                (int16_t)(at + 0x3d), 0xe0, 0);

    restore_cursor_following();
}

/*
 * 0x11c6b
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b3.
 *
 * The knob's x is `0x50b3 * 0xa0 / 0x80 + 0x3d`, worked out as a long -
 * `mul16x16` then `long_divide` - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x80
 * against 0x200, so they are not the same slider at two positions.
 */
void paint_panel_g(void)
{
    int16_t at;

    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x10)), 0x41, 0x114, 0);
    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x12)), 0x3d, 0x131, 0);

    at = (int16_t)long_divide(
             (int32_t)mul16x16(DG16(0x50b3), 0xa0), 0x80);

    draw_bitmap(DGU16((uint16_t)(DGU16(0x52f4) + 0x0c)),
                (int16_t)(at + 0x3d), 0x12c, 0);

    restore_cursor_following();
}

/*
 * 0x11632
 *
 * **Paint the game screen**: the play area, the control panel down the left,
 * and the three ornaments that sit on it.
 *
 * The order is the order the pieces overlap in. The play area is cleared to
 * the colour at DGROUP 0x52cb - `fill_rect(8, 8, 0x230, 0x160)`, inside the
 * clip box `set_clip_play_area` just set - then the machine is drawn over it,
 * then the panel at `draw_panel(0x2c, 0x42, 0xd0, 0x109)` and its contents.
 *
 * The panel's contents are eleven separate painters, each of which takes a
 * flag this passes as zero, and the flag is presumably "redraw only". Four
 * always run; then the fork on 0x4e67 - the same word `round_setup` uses to
 * tell free play from a level - chooses **two** painters for free play and
 * **one** for a level. That is the control panel having a different set of
 * controls in the two modes.
 *
 * The three bitmaps at the end come from the set at 0x52f4, at +6, +0xa and
 * +8, placed at (0x53,0x42), (0x64,0xb2) and (0x5b,0xfe) - note the middle one
 * is +0xa and the last +8, which is not the order they are drawn in.
 *
 * `select_music` is given the level's own tune from 0x50bb, which
 * `read_level` filled in.
 *
 * The argument decides whether the finished screen is presented: non-zero
 * calls 0x081f9. So a caller can paint into the back page and show it, or
 * paint and leave it for something else to show.
 */
void paint_game_screen(uint16_t present)
{
    uint16_t set;

    wait_cursor();
    set_clip_play_area();

    DGU16(0x38a8) = DGU16(0x38a2);
    DG8(0x389d) = DG8(0x52cb);
    DG8(0x389e) = DG8(0x52cb);
    DG8(0x389c) = 1;

    clear_flag_2d44_thunk();
    fill_rect(8, 8, 0x230, 0x160);

    draw_machine_thunk();
    paint_panel_frame();

    draw_panel(0x2c, 0x42, 0xd0, 0x109);

    paint_panel_a(0);
    paint_panel_b(0);
    paint_panel_c(0);
    paint_panel_d(0);

    if (DGU16(0x4e67) != 0) {
        paint_panel_free_a(0);
        paint_panel_free_b(0);
    } else {
        paint_panel_level(0);
    }

    paint_panel_e();
    paint_panel_f();
    paint_panel_g();

    clear_flag_2d44_thunk();
    set = DGU16(0x52f4);
    draw_bitmap(DGU16((uint16_t)(set + 6)), 0x53, 0x42, 0);
    draw_bitmap(DGU16((uint16_t)(set + 0xa)), 0x64, 0xb2, 0);
    draw_bitmap(DGU16((uint16_t)(set + 8)), 0x5b, 0xfe, 0);
    restore_cursor_following();

    select_music(DG16(0x50bb));

    if (present != 0)
        present_back_page();

    restore_cursor();
}

/*
 * 0x12269
 *
 * **Read a level file.** The name is opened, checked, unpacked field by field
 * into DGROUP, and closed; a file that does not open leaves everything as it
 * was and only the last line runs.
 *
 * The first word must be **0xaced** or the whole of the rest is skipped - the
 * file is still closed, and 0x50d3 is still pointed at the parts list, so a
 * corrupt level leaves the game with an empty machine rather than half of a
 * broken one.
 *
 * The flag at 0x5472 that `load_level` sets is what tells a *level* from a
 * saved machine. Set, the file also carries its title and hint at 0x4ecf and
 * 0x4f1f, the two counters at 0x50af and 0x50b1, and the origin pair at 0x50b7
 * and 0x50b9. Clear, all six are left as they are and only the parts are read.
 * So the same reader serves both, and one word decides which.
 *
 * The gravity and air pressure at 0x50b3 and 0x50b5 are always read, and
 * `recompute_kind_physics` is called immediately after them - not at the end -
 * so the three lists that follow are built against the settings the file
 * asked for rather than the ones the last level left behind.
 *
 * Three counts then arrive together and their **sum** is what the part table
 * is allocated for, once, before any of the three lists is read. The lists are
 * the machine's own parts at 0x521b, the moving ones at 0x5179, and - only
 * when 0x5472 says this is a level - the parts the player is given, at 0x50d7.
 *
 * The far pointer at 0x546c is freed at the end - whatever the list reader
 * left there - and a 0x216-byte buffer on the stack is handed to the file
 * first, which is a
 * `setvbuf` and nothing to do with the level's contents.
 */
void read_level(uint16_t name)
{
    uint16_t fp  = dg_enter(0x216);
    uint16_t buf = fp;
    uint16_t file;
    int16_t  n_machine, n_moving, n_given;

    file = game_fopen(name, 0x2870);
    if (file == 0) {
        DGU16(0x50d3) = 0x50d7;
        dg_leave(0x216);
        return;
    }

    stdio_setbuf_for(file, buf);
    game_fread_far(file, 0x5476);

    if (DGU16(0x5476) == 0xaced) {
        game_fread_far(file, 0x5474);

        if (DGU16(0x5472) != 0) {
            game_fread_string(file, 0x4ecf);
            game_fread_string(file, 0x4f1f);
            game_fread_far(file, 0x50af);
            game_fread_far(file, 0x50b1);
        }

        game_fread_far(file, 0x50b3);
        game_fread_far(file, 0x50b5);
        recompute_kind_physics();

        if (DGU16(0x5472) != 0) {
            game_fread_far(file, 0x50b7);
            game_fread_far(file, 0x50b9);
        }

        game_fread_far(file, 0x50bb);

        game_fread_far(file, (uint16_t)(fp + 0x214));
        game_fread_far(file, (uint16_t)(fp + 0x212));
        game_fread_far(file, (uint16_t)(fp + 0x210));
        n_machine = DG16((uint16_t)(fp + 0x214));
        n_moving  = DG16((uint16_t)(fp + 0x212));
        n_given   = DG16((uint16_t)(fp + 0x210));

        DGU16(0x5470) = 0;
        alloc_part_table((int16_t)(n_machine + n_moving + n_given));

        read_list(file, 0x521b, n_machine);
        read_list(file, 0x5179, n_moving);
        if (DGU16(0x5472) != 0)
            read_list(file, 0x50d7, n_given);

        dos_free_far(DGU16(0x546c), DGU16(0x546e));
    }

    game_fclose(file);
    DGU16(0x50d3) = 0x50d7;

    dg_leave(0x216);
}

/*
 * 0x0f0b0
 *
 * NOT TRANSCRIBED YET. Answers something about the level that "leave freeform
 * mode" consults before deciding to load it again - it takes copies of DGROUP
 * 0x4eaf and 0x4ead, files the level number 0x4ebd at 0x542a, calls 0xf499 and
 * files that at 0x542c, then compares 0x4ebd against 0x4eb7. What it decides is
 * not established, so it keeps its address for a name.
 */
uint16_t sub_0f0b0(void)
{
    not_transcribed("0x0f0b0");
    return 0;
}

/*
 * 0x0f0a6
 *
 * NOT TRANSCRIBED YET. Takes a round down, after `game_round`'s loop ends.
 */
void round_teardown(void)
{
    not_transcribed("0x0f0a6");
}

/*
 * 0x10fde
 *
 * **The volume knob, upwards.** State 0x4000, and the counterpart at 0x11025
 * is the same thing downwards.
 *
 * It is an *auto-repeat*, which is why it needs the loop's own counter. While
 * the button state at DGROUP 0x5774 is 1 or 2 - held - `held` counts passes,
 * and the step is taken only on every eighth one, so holding the knob down
 * walks the level up at a readable rate rather than as fast as the screen
 * loops. Let go and 0x5774 is neither, which resets the counter and puts the
 * state back to 2, the screen's resting state.
 *
 * The level is DGROUP 0x4ec1, and it stops at 6 rather than wrapping. Each
 * step redraws through 0x12bed and then sets the master level from the table
 * at DGROUP 0x116, indexed by the level - so the table is what the knob's
 * seven positions *mean*, and the knob itself only counts.
 *
 * The tail sets `repaint_e` to **2 and not 1**, which is the double buffer: one
 * per page, so the knob does not draw itself into whichever page happens to be
 * current and leave the other a frame stale.
 */
void screen_state_4000(struct screen_loop *s)
{
    if (DGU16(0x5774) != 1 && DGU16(0x5774) != 2) {
        s->held = 0;
        DGU16(0x4e6b) = 2;
    } else {
        if (s->held % 8 == 0 && DG16(0x4ec1) != 6) {
            DG16(0x4ec1)++;
            sub_12bed();
            set_master_level_ok(DGU16((uint16_t)(0x116 + 2 * DGU16(0x4ec1))));
        }
        s->held++;
    }

    s->repaint_e = 2;
}

/*
 * 0x11025
 *
 * **The volume knob, downwards.** State 0x2000: 0x10fde with `dec` for `inc`
 * and a floor of 0 for its ceiling of 6. Transcribed as its own routine
 * because the original has two, and which one a state reaches is the whole
 * difference between them.
 */
void screen_state_2000(struct screen_loop *s)
{
    if (DGU16(0x5774) != 1 && DGU16(0x5774) != 2) {
        s->held = 0;
        DGU16(0x4e6b) = 2;
    } else {
        if (s->held % 8 == 0 && DG16(0x4ec1) != 0) {
            DG16(0x4ec1)--;
            sub_12bed();
            set_master_level_ok(DGU16((uint16_t)(0x116 + 2 * DGU16(0x4ec1))));
        }
        s->held++;
    }

    s->repaint_e = 2;
}

/*
 * 0x11072
 *
 * **Quit game.** State 0x1000: draw the panel piece that shows the button
 * pressed, put that on the screen, and ask.
 *
 * `present_back_page` before the question and not after: the player has to see
 * the button go down before a box appears over it, and the box is drawn into
 * the page that is now the back one.
 *
 * Yes leaves by writing 1 into the state and setting `done`, which is the only
 * way out of `game_screen`'s loop - it does not return a value. No puts the
 * state back to 2 and asks for one whole-screen repaint, which is what paints
 * over where the box was.
 */
void screen_state_1000(struct screen_loop *s)
{
    paint_panel_b(1);
    present_back_page();

    if (ask_yes_no(0x1da5, 0x1daf)) {   /* "QUIT GAME" / "Are you sure ..." */
        DGU16(0x4e6b) = 1;
        s->done = 1;
    } else {
        DGU16(0x4e6b) = 2;
        s->repaint_all = 1;
    }
}

/*
 * 0x110ad
 *
 * **Restart level.** State 0x0800: the same shape as quit - draw the button
 * pressed, present it, ask - and the same two ways out.
 *
 * Yes clears the machine through `remove_all_parts` and then leaves the loop
 * with the state at **0x1000**, not 1. 0x1000 is quit's state, so a restart
 * goes out the way a quit does and `game_round` is what tells them apart; this
 * routine does not restart anything itself.
 */
void screen_state_0800(struct screen_loop *s)
{
    paint_panel_c(1);
    present_back_page();

    if (ask_yes_no(0x1dd7, 0x1de5)) {   /* "RESTART LEVEL" */
        remove_all_parts();
        DGU16(0x4e6b) = 0x1000;
        s->done = 1;
    } else {
        DGU16(0x4e6b) = 2;
        s->repaint_all = 1;
    }
}

/*
 * 0x110ed
 *
 * **Freeform mode.** State 0x0400, and it does nothing at all if DGROUP 0x4e67
 * says the game is already in it - the test at 0x110f2 jumps past everything,
 * including the repaint, to the common tail.
 *
 * Otherwise it asks, and yes tears the round down, loads `ff.lev`, resets the
 * machine and clears five words: the mode flag 0x4e67 goes to 1 and 0x4eaf,
 * 0x4ead, 0x50b1 and 0x50af to zero. `start_counters` last.
 *
 * **The state goes to 2 on every path**, including the one that did nothing,
 * because all three converge on 0x11321 - so this cannot be left half-entered.
 * The whole-screen repaint is asked for on the two that got as far as the
 * question, and not on the early exit.
 */
void screen_state_0400(struct screen_loop *s)
{
    if (DGU16(0x4e67) != 0) {
        DGU16(0x4e6b) = 2;
        return;
    }

    paint_panel_level(1);
    present_back_page();

    if (ask_yes_no(0x1e26, 0x1e34)) {   /* "FREEFORM MODE" */
        round_teardown();
        load_animation(0x2824);         /* "ff.lev" */
        reset_machine();

        DGU16(0x4e67) = 1;
        DGU16(0x4eaf) = 0;
        DGU16(0x4ead) = 0;
        DGU16(0x50b1) = 0;
        DGU16(0x50af) = 0;

        start_counters();
    }

    s->repaint_all = 1;
    DGU16(0x4e6b) = 2;
}

/*
 * 0x1114f
 *
 * **Leave freeform mode**, and reload the level if it needs it. State 0x0200.
 *
 * Two tests on DGROUP 0x4e67, not one, and they are not the same test. The
 * first asks whether the game is *in* freeform mode and only then puts the
 * question; answering yes clears the flag and sets `reload`. The second asks
 * again, having possibly just cleared it - so the reload below runs both for
 * someone who has this moment left freeform mode and for someone who was never
 * in it. Reading the second as an `else` of the first loses that.
 *
 * The reload itself is conditional on either `sub_0f0b0` answering non-zero or
 * `reload` being set, and clears `reload` on its way out.
 *
 * The state goes to 2 and the screen repaints whole, on every path, at 0x11321
 * - the same convergence "freeform mode" uses.
 */
void screen_state_0200(struct screen_loop *s)
{
    paint_panel_d(1);
    present_back_page();

    if (DGU16(0x4e67) != 0) {
        if (ask_yes_no(0x1e62, 0x1e76)) {   /* "LEAVE FREEFORM MODE" */
            DGU16(0x4e67) = 0;
            s->reload = 1;
        }
    }

    if (DGU16(0x4e67) == 0) {
        if (sub_0f0b0() != 0 || s->reload != 0) {
            round_teardown();
            load_level(DGU16(0x4ebd));
            reset_machine();
            s->reload = 0;
            start_counters();
        }
    }

    s->repaint_all = 1;
    DGU16(0x4e6b) = 2;
}

/*
 * 0x111bd
 *
 * **Load a machine**, and freeform only: the test on DGROUP 0x4e67 at the top
 * jumps straight to the common tail if the game is not in freeform mode, so
 * the button exists on every screen and does nothing on most of them.
 *
 * The picker is bracketed by two **directory dances**, and they are not the
 * same one. Before: change to the path at DGROUP 0x530b and, if that worked,
 * select the drive its first character names - which is the game's own
 * directory being restored. After: `dos_get_cur_dir` writes wherever the
 * picker left the process into 0x530b, and the second pair does the same dance
 * with 0x535b. So the picker is free to wander and the game puts itself back.
 *
 * `0x4e85` is 1 around each dance and 0 between them, which is the only thing
 * that distinguishes "the game is doing file IO" from the rest of the loop.
 *
 * Choosing a file tears the round down, loads it - `load_animation` with the
 * name at 0x52fe, which is where the picker left it - and resets the machine.
 * Choosing nothing does none of that, and either way the screen repaints whole
 * and the state returns to 2.
 */
void screen_state_0100(struct screen_loop *s)
{
    if (DGU16(0x4e67) == 0) {
        DGU16(0x4e6b) = 2;
        return;
    }

    paint_panel_free_a(1);
    present_back_page();

    DGU16(0x4e85) = 1;
    if (dos_chdir(0x530b) == 0)
        dos_setdisk(DG8(0x530b));
    DGU16(0x4e85) = 0;

    if (pick_file(0x282b, 0, 0)) {      /* "*.TIM" */
        round_teardown();
        load_animation(0x52fe);
        reset_machine();
    }

    DGU16(0x4e85) = 1;
    dos_get_cur_dir(0x530b);
    if (dos_chdir(0x535b) == 0)
        dos_setdisk(DG8(0x535b));
    DGU16(0x4e85) = 0;

    s->repaint_all = 1;
    DGU16(0x4e6b) = 2;
}

/*
 * 0x11258
 *
 * **Save the machine**, freeform only, and it is a *retry loop* - the one
 * handler here that is. The picker is asked, the file written, and if the write
 * answers anything but zero the player gets "FILE ERROR" and is asked again.
 * Cancelling the picker sets the result to zero, which is what ends the loop:
 * the same word means "no error" and "nothing to do", and the loop cannot tell
 * them apart because it does not need to.
 *
 * The state is written back to 0x80 at the top of every pass - the state it is
 * already in - so the screen underneath keeps showing the save button pressed
 * while the picker is up.
 *
 * The error box repaints the game screen with `paint_game_screen(0)`, a **0 and
 * not a 1**, which is the argument that says do not present it; the loop is
 * about to put the picker up again over the top.
 *
 * Two directory dances around the whole thing, as in 0x111bd - see there for
 * what they are.
 */
void screen_state_0080(struct screen_loop *s)
{
    if (DGU16(0x4e67) == 0) {
        DGU16(0x4e6b) = 2;
        return;
    }

    paint_panel_free_b(1);
    present_back_page();

    DGU16(0x4e85) = 1;
    if (dos_chdir(0x530b) == 0)
        dos_setdisk(DG8(0x530b));
    DGU16(0x4e85) = 0;

    s->file_err = 1;
    while (s->file_err != 0) {
        DGU16(0x4e6b) = 0x80;

        if (pick_file(0x2831, 0, 0)) {          /* "*.TIM" */
            s->file_err = save_machine(0x52fe);
            if (s->file_err != 0) {
                show_message_box(0x1fa0, 0x1ff6);   /* "FILE ERROR" */
                paint_game_screen(0);
            }
        } else {
            s->file_err = 0;
        }
    }

    dos_get_cur_dir(0x530b);
    DGU16(0x4e85) = 1;
    if (dos_chdir(0x535b) == 0)
        dos_setdisk(DG8(0x535b));
    DGU16(0x4e85) = 0;

    s->repaint_all = 1;
    DGU16(0x4e6b) = 2;
}

/*
 * 0x1132a
 *
 * **The gravity slider**, and it is the exact inverse of the knob
 * `paint_panel_f` draws. That routine puts the knob at
 * `0x50b5 * 0xa0 / 0x200 + 0x3d`; this takes the pointer's x at DGROUP 0x5784,
 * subtracts 67 - the `add ax, 0xffbd` - multiplies by 0x200 and divides by
 * 0xa0. The two agree by construction, so the knob lands under the pointer.
 *
 * The multiply and the divide are a long, because `x * 0x200` leaves a word
 * before the divide brings it back, which is the same reason `paint_panel_f`
 * uses one.
 *
 * Outside freeform mode it refuses, with a box saying so, and asks for a whole
 * repaint to paint over it. Inside, it only acts while the button is held -
 * this is a *drag*, not a click - and letting go returns the state to 2.
 *
 * The clamp is to 0 and 0x200 and it is done on the value, not the pointer, so
 * dragging past either end of the track pins the knob rather than stopping the
 * drag. And the store is guarded by `!=`: an unchanged value neither writes
 * 0x50b5 nor asks for the repaint, so holding the knob still costs nothing.
 *
 * `recompute_kind_physics` afterwards, because gravity is not a display value -
 * every kind's behaviour is derived from it.
 */
void screen_state_0040(struct screen_loop *s)
{
    int32_t v;

    if (DGU16(0x4e67) == 0) {
        /* "CAN'T CHANGE GRAVITY" */
        show_message_box(0x1ea4, 0x1eb9);
        s->repaint_all = 1;
        DGU16(0x4e6b) = 2;
        return;
    }

    if (DGU16(0x5774) != 1 && DGU16(0x5774) != 2) {
        DGU16(0x4e6b) = 2;
        return;
    }

    v = long_divide((int32_t)mul16x16((int16_t)(DG16(0x5784) - 67), 0x200),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x200)
        v = 0x200;

    if ((int16_t)v != DG16(0x50b5)) {
        DG16(0x50b5) = (int16_t)v;
        s->repaint_f = 2;
        recompute_kind_physics();
    }
}

/*
 * 0x113c3
 *
 * **The air-pressure slider.** 0x1132a with three numbers changed: the scale is
 * 0x80 rather than 0x200, the value goes to DGROUP 0x50b3 rather than 0x50b5,
 * and it is `repaint_g` that is asked for. The x is taken the same way, less
 * the same 67, over the same 0xa0 of track - and `paint_panel_g` divides by
 * 0x80 where `paint_panel_f` divides by 0x200, which is the same pair of
 * numbers seen from the drawing side.
 *
 * Transcribed as its own routine rather than folded into the gravity one with
 * the differences as arguments. The original has two, reached by two states,
 * and what a screen does is decided by which one it reaches.
 */
void screen_state_0020(struct screen_loop *s)
{
    int32_t v;

    if (DGU16(0x4e67) == 0) {
        /* "CAN'T CHANGE AIR PRESSURE" */
        show_message_box(0x1f02, 0x1f1c);
        s->repaint_all = 1;
        DGU16(0x4e6b) = 2;
        return;
    }

    if (DGU16(0x5774) != 1 && DGU16(0x5774) != 2) {
        DGU16(0x4e6b) = 2;
        return;
    }

    v = long_divide((int32_t)mul16x16((int16_t)(DG16(0x5784) - 67), 0x80),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x80)
        v = 0x80;

    if ((int16_t)v != DG16(0x50b3)) {
        DG16(0x50b3) = (int16_t)v;
        s->repaint_g = 2;
        recompute_kind_physics();
    }
}

/*
 * 0x1156c
 *
 * NOT TRANSCRIBED YET. What `game_screen` calls when the key it just read
 * has scancode 0x0f - Tab.
 */
void sub_1156c(void)
{
    not_transcribed("0x1156c");
}

/*
 * 0x1567b
 *
 * **A message box with two buttons**, answering which was pressed. The other
 * doorway into 0x15698, twenty-six bytes past the first, and the only
 * difference is that both button strings are given: 0x25e1 and 0x25e5.
 *
 * Quit, restart and both freeform handlers ask through this one. It is its own
 * routine and not an argument to `show_message_box` because that is what the
 * original has - two entry points to one body, the way Borland's runtime is
 * built and the way the part tables reach shared code.
 *
 * Its `jmp` to the instruction after it, at 0x15694, is the compiler leaving a
 * return path in that nothing needed; transcribed as the fall-through it is.
 */
uint16_t ask_yes_no(uint16_t title, uint16_t body)
{
    return message_box(title, body, 0x25e1, 0x25e5);
}

/*
 * 0x15698
 *
 * **The message box.** Both doorways above reach it - `show_message_box` with
 * one button and `ask_yes_no` with two - and it answers 1 for the first button
 * and 0 for the second or for none.
 *
 * **It takes the screen over by borrowing the state word.** DGROUP 0x4e6b is
 * what `game_screen` and `game_round` dispatch on; it is saved, set to 0x8000
 * while the box is up, and put back on the way out. So the box's own loop tests
 * the same word those screens do, 0x4000 and 0x2000 mean its two buttons here,
 * and nothing underneath can act on a click meant for it.
 *
 * **The buttons' keys come from their first letter.** `[si]` is the first byte
 * of the first button's string, and the shortcuts are chosen from it: 'Y' takes
 * Y for the first button and N for the second, 'R' takes R and A, 'C' takes C -
 * and Enter, which is the only key that means the same as a button rather than
 * naming one. So "YES"/"NO" and "CONTINUE" get their keys without a table, and
 * a button whose word began with something else would get none.
 *
 * **The second button is right-aligned by measurement**: its width is rounded
 * up to a multiple of 8 and taken from 0x168, and that x is filed into the
 * region record at [0x4e6d]+6 so the clickable area moves with it. The first
 * button's own width plus 0xd8 goes into [0x4e6f]+0xa the same way. A box with
 * one button files only the first.
 *
 * **One button means the second cannot be chosen**: with `di` zero, a state of
 * 0x2000 is turned straight back into 0x8000 at 0x15814, so the loop carries on
 * rather than leaving with an answer nothing asked for.
 *
 * On the way out the chosen button is drawn again pressed and presented, which
 * is what makes it flash before the box goes.
 */
uint16_t message_box(uint16_t title, uint16_t body,
                     uint16_t button1, uint16_t button2)
{
    uint16_t saved;
    int16_t  second_x = 0;

    wait_cursor();

    saved = DGU16(0x4e6b);
    DGU16(0x4e6b) = 0x8000;

    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(title, 0xb8, 0x74, 0xd0);
    draw_panel(0xb8, 0x90, 0xd0, 0x5a);
    draw_wrapped_text(body, 0xbc, 0x94, 0xc8, 0x30);

    draw_button(button1, 0xc8, 0xd4, 0);
    DGU16((uint16_t)(DGU16(0x4e6f) + 0x0a)) =
        (uint16_t)(text_width_thunk(button1) + 0xd8);

    if (button2 != 0) {
        second_x = (int16_t)(0x168
                             - ((text_width_thunk(button2) + 7) & 0xfff8));
        draw_button(button2, (uint16_t)second_x, 0xd4, 0);
        DGU16((uint16_t)(DGU16(0x4e6d) + 6)) = (uint16_t)second_x;
    }

    present_back_page();
    restore_cursor();

    while (DGU16(0x4e6b) == 0x8000) {
        update_button_state();

        DG8(0x52f1) = (uint8_t)(bios_read_key() >> 8);

        if (DG8(0x52f1) == 0x0f) {
            message_box_tab(button2);
        } else {
            if (DG8(button1) == 'Y') {
                if (DG8(0x52f1) == 0x15)
                    DGU16(0x4e6b) = 0x4000;
                if (DG8(0x52f1) == 0x31)
                    DGU16(0x4e6b) = 0x2000;
            }
            if (DG8(button1) == 'R') {
                if (DG8(0x52f1) == 0x13)
                    DGU16(0x4e6b) = 0x4000;
                if (DG8(0x52f1) == 0x1e)
                    DGU16(0x4e6b) = 0x2000;
            }
            if (DG8(button1) == 'C') {
                if (DG8(0x52f1) == 0x2e)
                    DGU16(0x4e6b) = 0x4000;
                if (DG8(0x52f1) == 0x1c)
                    DGU16(0x4e6b) = 0x4000;
            }
        }

        regions_handle_pointer(DGU16(0x4e73));

        if (button2 == 0 && DGU16(0x4e6b) == 0x2000)
            DGU16(0x4e6b) = 0x8000;

        present_frame(1);
    }

    update_button_state();

    if (DGU16(0x4e6b) == 0x4000) {
        draw_button(button1, 0xc8, 0xd4, 1);
        present_back_page();
        DGU16(0x4e6b) = saved;
        return 1;
    }

    if (button2 != 0) {
        draw_button(button2, (uint16_t)second_x, 0xd4, 1);
        present_back_page();
    }
    DGU16(0x4e6b) = saved;
    return 0;
}

/*
 * 0x1588c
 *
 * **Tab walks the pointer between the buttons.** A counter at DGROUP 0x259c
 * steps on each press and the pointer is moved to the x that counter names in
 * the table at 0x259e - 232 for the first button, 360 for the second - at a
 * fixed y of 0xde.
 *
 * With no second button the counter is put straight back to zero, so Tab keeps
 * the pointer on the only button there is rather than sending it to where the
 * other one would have been. With one, it wraps at 2.
 *
 * It moves the *pointer*, not a highlight: there is no selected button in this
 * box, only where the mouse is, and Tab is a way of driving the mouse from the
 * keyboard.
 */
void message_box_tab(uint16_t button2)
{
    DGU16(0x259c)++;

    if (button2 != 0) {
        if (DGU16(0x259c) == 2)
            DGU16(0x259c) = 0;
    } else {
        DGU16(0x259c) = 0;
    }

    move_pointer_to((int16_t)DGU16((uint16_t)(0x259e + 2 * DGU16(0x259c))),
                    0xde);
}


/*
 * 0x15661
 *
 * **A message box with one button.** It is a doorway: the box itself is
 * 0x15698, and this passes it the title, the body, "CONTINUE" for the first
 * button and **zero for the second**, which is how the box is told there is
 * only one.
 *
 * The zero is pushed first and the strings after, so what the box reads as its
 * fourth argument is the absent button rather than a flag saying how many there
 * are. That is the whole difference between this and `ask_yes_no` below.
 */
void show_message_box(uint16_t title, uint16_t body)
{
    message_box(title, body, 0x25d8, 0);        /* "CONTINUE" */
}

/*
 * 0x10f03
 *
 * **The game screen.** State 2 dispatches here, so this is the first screen a
 * round shows - the level briefing for round 1 - and it stays here, running
 * its own loop, until something sets the state to one it does not handle.
 *
 * It paints once on the way in, through `paint_game_screen(1)`, and then loops:
 * take the button state and a key, let the regions at DGROUP 0x4e77 see the
 * pointer, and dispatch on the state at 0x4e6b through a **jump table** at
 * CS:0x34bf - eleven single-bit states, 0x0020 through 0x8000, with the
 * handlers in a second table 0x16 bytes further on.
 *
 * State 2 is *not* in that table. The search runs off the end and falls to the
 * bottom of the loop, so the screen simply sits and presents itself, which is
 * what a briefing waiting for a click is.
 *
 * **The repaint counters are counts and not flags, and that is the double
 * buffer showing through.** `si` repaints the whole screen and the three at
 * [bp-0xa], [bp-0xc] and [bp-0xe] repaint one panel piece each; every one is
 * decremented by one per pass rather than cleared, so setting a counter to two
 * paints the same thing into both pages. A flag would paint it into whichever
 * page happened to be current and leave the other stale for a frame.
 *
 * The whole-screen repaint and the piecewise ones are exclusive - `or si,si`
 * takes the first branch - so a full repaint does not also run the three.
 *
 * Alt and V together put up a version box: `bit0_of_468c` is asked for
 * scancodes 0x38 and 0x2f, and both being down shows it and asks for a full
 * repaint afterwards.
 *
 * The eleven handlers are stubs. Each is named for the state that reaches it,
 * because that is what is known about it; what each screen *is* is not, and
 * naming them for their addresses would lose even that.
 */
void game_screen(void)
{
    uint16_t fp = dg_enter(0x16);
    struct screen_loop s = {0, 0, 0, 0, 0, 0, 0, 0};

    (void)fp;

    reset_machine();
    paint_game_screen(1);
    set_palette_pointer(DGU16(0x52ed), DGU16(0x52ef));
    show_cursor_again();

    while (s.done == 0) {
        update_button_state();

        DG8(0x52f1) = (uint8_t)(bios_read_key() >> 8);
        if (DG8(0x52f1) == 0x0f)
            sub_1156c();

        regions_handle_pointer(DGU16(0x4e77));

        if (bit0_of_468c(0x38) && bit0_of_468c(0x2f)) {
            show_message_box(0x1cee, 0x1cfd);   /* "VERSION NUMBER" */
            s.repaint_all = 1;
            DGU16(0x4e6b) = 2;
        }

        switch (DGU16(0x4e6b)) {
        case 0x8000:
            paint_panel_a(1);
            present_back_page();
            DGU16(0x4e6b) = 0x1000;
            s.done = 1;
            break;
        case 0x4000: screen_state_4000(&s); break;
        case 0x2000: screen_state_2000(&s); break;
        case 0x1000: screen_state_1000(&s); break;
        case 0x0800: screen_state_0800(&s); break;
        case 0x0400: screen_state_0400(&s); break;
        case 0x0200: screen_state_0200(&s); break;
        case 0x0100: screen_state_0100(&s); break;
        case 0x0080: screen_state_0080(&s); break;
        case 0x0040: screen_state_0040(&s); break;
        case 0x0020: screen_state_0020(&s); break;
        default:
            /* State 2 among them: not in the table, so nothing runs. */
            break;
        }

        if (s.repaint_all != 0) {
            paint_game_screen(1);
            s.repaint_all--;
        } else {
            if (s.repaint_e != 0) {
                paint_panel_e();
                s.repaint_e--;
            }
            if (s.repaint_f != 0) {
                paint_panel_f();
                s.repaint_f--;
            }
            if (s.repaint_g != 0) {
                paint_panel_g();
                s.repaint_g--;
            }
        }

        present_frame(1);
    }

    dg_leave(0x16);
}

/*
 * 0x0f8c2
 *
 * NOT TRANSCRIBED YET. **The machine running** - the screen every state but 2
 * and 0x2000 dispatches to, and what the panel's play triangle reaches. It is
 * the next thing between the level-one briefing and the level actually
 * playing.
 *
 * Read, and the parts worth having before writing it. This is a *subsystem*,
 * not a routine: what follows is the shape, not a transcription plan that can
 * be finished in one sitting.
 *
 * **It is a loop on the same DGROUP 0x4e6b that `game_round` dispatches on.**
 * 0x0fa91 is its test - it stays only while the state is 0x2000 or 2 - and the
 * body runs from 0x0f8d6. So the round's state machine and this loop share one
 * word, and a screen leaves by writing into it rather than by returning.
 *
 * **0x44ef is reloaded with 0x2710 twice**: once on entry at 0x0f8cd and again
 * at 0x0fa6f, after a wait that spins until the timer has counted eight off it
 * (0x0fa66: `sub ax, [0x44ef]` against 8). That is the frame pacing, and it is
 * the same counter whose value decides the copy-protection page - see
 * STATUS.md, where the two clocks are why five flips of the run differ.
 *
 * **Five deferred redraws**, each a countdown and a handler, at 0x4e93, 0x4e91,
 * 0x4e8f, 0x4e8d and 0x4e8b, calling VMDS 0x14de:0x101d, 0xd36, 0xdbf, 0xe33
 * and 0xea3. Each is "if the count is not zero, redraw and decrement", so a
 * change to a panel asks for N frames of redraw rather than drawing once.
 *
 * **What it calls that is not here yet**: 0x0fbda on entry, 0x0fd65, 0x0faf9
 * (free play only, gated on 0x4e67), 0x10cc8 and 0x10d37 (states 0x800 and
 * 0x400), 0x0fc0e, 0x0fd02. Plus 0x08546, 0x080b9, 0x0647f, 0x06806, 0x06699,
 * 0x02510 and the driver-side 0x14de:0x13a1 and 0x1429. Whether those are
 * transcribed has not been checked routine by routine.
 *
 * Reached with `TIM_CLICK=200:320:200,400:78:105` - the menu, then the play
 * triangle - which is also what exercised `dev_final_frame`.
 */
void sub_0f8c2(void)
{
    not_transcribed("0x0f8c2");
}

/*
 * 0x12bed
 *
 * NOT TRANSCRIBED YET. Called when the count at DGROUP 0x4ebd passes the best
 * at 0x4eb7, which is the shape of a high score being written out.
 */
void sub_12bed(void)
{
    not_transcribed("0x12bed");
}

/*
 * 0x0efdc
 *
 * Give back the two bitmap lists the game keeps at DGROUP 0x4ecd and 0x4ec9,
 * in that order, through the driver's own thunk.
 */
void free_two_bitmap_lists(void)
{
    free_bitmaps_thunk(DGU16(0x4ecd));
    free_bitmaps_thunk(DGU16(0x4ec9));
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
 * 0x0f86e
 *
 * Give back every part's bitmaps: 0 to 0x39, one at a time, and no skipping -
 * unlike `load_all_parts`, which leaves out 10 and 0x31 because there is no
 * part with those numbers. Freeing one that was never loaded is harmless, so
 * the loop is written plainly.
 */
void free_all_part_bitmaps(void)
{
    int16_t si;

    for (si = 0; si < 0x3a; si++)
        free_part_bitmap((uint16_t)si);
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
 * 0x12c26
 *
 * NOT TRANSCRIBED YET. **The file picker.** Given a pattern - "*.TIM" for a
 * saved machine - and two more arguments that are zero at the one call site
 * read so far, it answers whether the player chose a file, leaving the name
 * where `load_animation` is then given it at DGROUP 0x52fe.
 */
uint16_t pick_file(uint16_t pattern, uint16_t a, uint16_t b)
{
    (void)pattern;
    (void)a;
    (void)b;
    not_transcribed("0x12c26");
    return 0;
}


/*
 * 0x123b7
 *
 * **Write one byte**, and do nothing at all once the file has gone wrong.
 *
 * The error word 0x5478 is checked first and every writer checks it, so a
 * failure part way through a machine file does not have to be propagated: the
 * remaining hundreds of calls simply become no-ops and `sub_1271c` finds the
 * word set when it gets to the end. That is why none of the writers answer
 * anything.
 */
void write_byte(uint16_t file, uint16_t addr)
{
    if (DGU16(0x5478) != 0)
        return;

    if (game_fwrite(addr, 1, 1, file) != 1)
        DGU16(0x5478) = 1;
}

/*
 * 0x123e4
 *
 * **Write one word.** The same routine as `write_byte` with a size of 2, and
 * the original writes it out twice rather than sharing one - so this does too.
 */
void write_word(uint16_t file, uint16_t addr)
{
    if (DGU16(0x5478) != 0)
        return;

    if (game_fwrite(addr, 2, 1, file) != 1)
        DGU16(0x5478) = 1;
}

/*
 * 0x12411
 *
 * **Write a string, and its terminator with it.** The loop writes the byte at
 * the pointer and *then* tests it, so the NUL goes to the file before the loop
 * ends - a reader has something to stop at. Written the other way round it
 * would be an off-by-one that only shows up when the file is read back.
 */
void write_string(uint16_t file, uint16_t str)
{
    for (;;) {
        write_byte(file, str);
        if (DG8(str) == 0)
            return;
        str++;
    }
}

/*
 * 0x12430
 *
 * NOT TRANSCRIBED YET. **Write one part.** The record `sub_126b3` puts down for
 * each part of a list, after the count `sub_126ec` wrote for that list.
 */
void sub_12430(uint16_t file, uint16_t part)
{
    (void)file;
    (void)part;
    not_transcribed("0x12430");
}

/*
 * 0x126b3
 *
 * **Write every part of one list, and mark it as it goes.**
 *
 * The mark is bit 15 of +6 - the same bit `remove_all_parts` refuses to touch a
 * part over. List 2 is the bin at 0x50d7 and every part in it has the bit
 * *cleared*; lists 0 and 1 have it *set*, but only when DGROUP 0x5472 says this
 * is the long form of the file. So saving is what decides which parts a reload
 * will call the level's own and which the player's, and in the short form -
 * which is what the game itself saves - nothing is marked at all.
 *
 * The bit is set on the live part and not on a copy, so a save leaves the
 * machine in memory marked as well as the file.
 */
void sub_126b3(uint16_t file, uint16_t head, uint16_t which)
{
    uint16_t p = DGU16(head);

    while (p != 0) {
        if (which == 2)
            DGU16((uint16_t)(p + 6)) &= 0x7fff;
        else if (DGU16(0x5472) != 0)
            DGU16((uint16_t)(p + 6)) |= 0x8000;

        sub_12430(file, p);
        p = DGU16(p);
    }
}

/*
 * 0x126ec
 *
 * **Write how many parts a list holds**, by walking it and counting.
 *
 * The count goes into a *stack* local whose address is then handed to
 * `write_word` - which is why the port takes a guest frame for it rather than
 * using a C variable. Every field of this file is written from an address, and
 * a count that exists only for the length of this call is no exception.
 *
 * This is the first of the two passes each list gets: the count first, so a
 * reader knows how many of the records that `sub_126b3` writes to expect.
 */
void sub_126ec(uint16_t file, uint16_t head)
{
    uint16_t fp = dg_enter(2);
    uint16_t vn = fp;                   /* [bp-2] */
    uint16_t p;

    DGU16(vn) = 0;
    for (p = DGU16(head); p != 0; p = DGU16(p))
        DGU16(vn)++;

    write_word(file, vn);

    dg_leave(2);
}

/*
 * 0x1271c
 *
 * **The machine file writer.** `save_machine` is the doorway that puts the
 * dragged part down first; this is what opens the file and writes it. Answers
 * zero on success.
 *
 * The file starts with 0xaced and then 0x0102, a magic and a version, and both
 * are written *out of DGROUP* - set into 0x5476 and 0x5474 first and the address
 * passed - because everything else here is written the same way and the writer
 * takes an address, not a value.
 *
 * **DGROUP 0x5472 decides how much goes in.** Two groups of fields are written
 * only when it is set - 0x4ecf and 0x4f1f, then 0x50af and 0x50b1, and later
 * 0x50b7 and 0x50b9 - while 0x50b3, 0x50b5 and 0x50bb always go. `save_machine`
 * zeroes 0x5472 before calling, so a machine saved from the game gets the short
 * form and only whatever else sets that word gets the long one.
 *
 * Then the three part lists - 0x521b, 0x5179 and 0x50d7 - each written twice:
 * once by `sub_126ec` and once by `sub_126b3`, which also takes 0, 1 and 2. Two
 * passes over the same three lists, so the second can refer to what the first
 * wrote; the tag says which list it is reading back.
 *
 * **A file that fails to close is deleted.** The error word 0x5478 is set by a
 * non-zero close as well as by a failed open, and a set error word deletes the
 * file - so a half-written machine does not survive to be loaded. The open
 * failing returns 1 without touching the disk.
 *
 * 0x4e85 is 1 across the whole of it, the same "doing file IO" mark the load and
 * save handlers set around the picker.
 */
uint16_t sub_1271c(uint16_t name)
{
    uint16_t f;

    DGU16(0x5478) = 0;
    DGU16(0x5476) = 0xaced;
    DGU16(0x5474) = 0x0102;
    DGU16(0x4e85) = 1;

    f = game_fopen(name, 0x2873);       /* "wb" */
    if (f == 0) {
        DGU16(0x4e85) = 0;
        return 1;
    }

    write_word(f, 0x5476);
    write_word(f, 0x5474);

    if (DGU16(0x5472) != 0) {
        write_string(f, 0x4ecf);
        write_string(f, 0x4f1f);
        write_word(f, 0x50af);
        write_word(f, 0x50b1);
    }

    write_word(f, 0x50b3);
    write_word(f, 0x50b5);

    if (DGU16(0x5472) != 0) {
        write_word(f, 0x50b7);
        write_word(f, 0x50b9);
    }

    write_word(f, 0x50bb);

    sub_126ec(f, 0x521b);
    sub_126ec(f, 0x5179);
    sub_126ec(f, 0x50d7);

    sub_126b3(f, 0x521b, 0);
    sub_126b3(f, 0x5179, 1);
    sub_126b3(f, 0x50d7, 2);

    if (game_fclose(f) != 0)
        DGU16(0x5478) = 1;

    if (DGU16(0x5478) != 0)
        dos_unlink(name);

    DGU16(0x4e85) = 0;
    return DGU16(0x5478);
}

/*
 * 0x1292d
 *
 * **Write the machine out**, given the name the picker left at DGROUP 0x52fe.
 * Answers zero on success - the caller shows "FILE ERROR" and asks again for
 * anything else, so what comes back is a reason and not a count.
 *
 * The writing is `sub_1271c`; what this adds is that **the dragged part is put
 * down first**. DGROUP 0x50d7 is saved, zeroed for the length of the write and
 * put back after, so a part in mid-drag is not written as held - the file has
 * no way to say "and this one is in the player's hand", and reloading it would
 * have to invent somewhere to put it. 0x5472 is zeroed with it and not restored.
 *
 * The `jmp` to the next instruction at 0x12959 is the compiler leaving itself a
 * single exit; transcribed as the fall-through it is.
 */
uint16_t save_machine(uint16_t name)
{
    uint16_t held = DGU16(0x50d7);
    uint16_t r;

    DGU16(0x50d7) = 0;
    DGU16(0x5472) = 0;

    r = sub_1271c(name);

    DGU16(0x50d7) = held;
    return r;
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
 * Throw the whole machine away: every part on the three lists at DGROUP
 * 0x50d7, 0x521b and 0x5179 is freed and the three heads cleared. The intro
 * calls it between one animation and the next, which is why the credits get a
 * clean machine rather than the title screen's leftovers.
 */
void free_all_lists(void)
{
    free_part_list(DGU16(0x50d7));
    free_part_list(DGU16(0x521b));
    free_part_list(DGU16(0x5179));

    DGU16(0x50d7) = 0;
    DGU16(0x5179) = 0;
    DGU16(0x521b) = 0;
}

/*
 * 0x14d71
 *
 * Free every part on one list. The next pointer is taken out of the record
 * *before* the record is freed, which is the only way to walk a list you are
 * destroying.
 */
void free_part_list(uint16_t p)
{
    while (p != 0) {
        uint16_t next = DGU16(p);

        free_part(p);
        p = next;
    }
}

