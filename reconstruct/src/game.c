/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game as a program**: `main`, the bring-up, the intro and the
 * copy protection, the round and screen state machines, the level loop and
 * the drag that places a part, and the teardown that prints your password.
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
    return game_teardown(1);
}

/*
 * 0x0e34a
 *
 * **Leaving the game.** `game_main`'s fourth call, and the one that actually
 * takes the program down.
 *
 * The argument is whether this is really the end. Called with 0 it only raises
 * DGROUP 0x52fa - a request to stop, which the loops above read - and returns.
 * Called with 1 it does the whole teardown and never comes back.
 *
 * **It prints your password on the way out.** If 0x4eb5 holds a puzzle number,
 * that puzzle's line of `password.txt` is read and `score_to_code` appends the
 * score kept at 0x4eab/0x4ea9 - the pair `finish_level` banks and only when the
 * puzzle was not the last. The message at DGROUP 0x1c49 goes in front of it and
 * the whole thing is handed to `printf` at the very end, after the screen has
 * been given back to DOS, so it is the last thing on the terminal.
 *
 * Then everything is handed back, in the original's order: a linked list of far
 * blocks whose first two words are the next pointer; a chain of near blocks
 * from 0x4e56; the five region lists; the part bitmaps; four bitmap lists;
 * a slot of the 0x618a table and three far blocks; the sound sequences,
 * records and driver; a file; the sound slots; and the keyboard, the rest of
 * the input and the video mode.
 *
 * `remove_keyboard` is called and then `shutdown_input` calls it again. The
 * second call finds the flag already clear and does nothing, which is what the
 * flag is for. Transcribed as the two calls it is.
 */
uint16_t game_teardown(int16_t really)
{
    uint8_t msg[240];                     /* [bp-0x122] */
    uint8_t code[50];  /* [bp-0x32]  */
    struct far_ptr node;
    uint16_t si;

    if (really == 0) {
        DG52ED.stop_requested = 1;
        return 0;
    }

    if (((uint16_t)DG4E67.password_puzzle) != 0) {
        read_password_line(DG4E67.password_puzzle, (volatile uint8_t *)code);
        score_to_code(DG4E67.score, (volatile uint8_t *)code);
        string_copy((volatile uint8_t *)msg, dg_ptr(dgroup, 0x1c49));
        string_concat((volatile uint8_t *)msg, (volatile uint8_t *)code);
    } else {
        (*msg) = 0;
    }

    node = DG4E4E.shape_free;
    while (!far_eq(node, FAR_NULL)) {
        struct far_ptr next;

        next.seg = (uint16_t)FAR16(node.seg, (uint16_t)(node.off + 2));
        next.off = (uint16_t)FAR16(node.seg, node.off);

        dos_free_far(node);
        node = next;
    }

    si = DG4E4E.parts_free_ptr;
    while (si != 0) {
        uint16_t next = QNODE(si).next;

        heap_free_far(dg_ptr(dgroup, si));
        si = next;
    }

    free_region_lists();
    free_all_part_bitmaps();

    free_bitmaps_thunk(BMPLIST(DG4E67.icons_bmp_ptr));
    free_bitmaps_thunk(BMPLIST(DG4E67.bmp_4ecb_ptr));
    free_bitmaps_thunk(BMPLIST(DG52ED.panel_art_ptr));
    free_bitmaps(BMPLIST(DG52ED.cursor_art_ptr));

    close_table_618a_slot(DG52BD.word_52df);

    free_far_block(DG52BD.pal_black_ptr.ptr);
    free_far_block(DG52BD.pal_sierra_ptr.ptr);
    free_far_block(DG52ED.pal_tim_ptr.ptr);

    stop_sequences(-2);
    remove_and_free_records(-2);
    shutdown_sound();

    close_file_record(DG52ED.word_52f8);
    free_archive_lists();

    remove_keyboard();
    shutdown_input();
    restore_video_mode();

    stdio_printf((volatile uint8_t *)msg);
    stdio_exit(0);
    return 0;
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
    uint8_t cfg_byte;

    int32_t free_bytes;
    int16_t sound_device, sound_module, cfg_first;
    uint16_t file, i;

    DG52ED.stack_floor = 0x800;

    free_bytes = (int32_t)dos_alloc_bytes(0xffffffffu, 0, 0).bytes;
    if (free_bytes < 0x00044d90L) {
        stdio_printf(dg_ptr(dgroup, 0x1bcc));       /* "\n\nNOT ENOUGH FREE MEMORY\n" */
        stdio_printf(dg_ptr(dgroup, 0x1be6));       /* "\nYou need at least 550k ..."  */
        stdio_exit(0);
    }

    dos_get_cur_dir(dg_off(dgroup, DG530B.game_dir));
    dos_get_cur_dir(dg_off(dgroup, DG530B.picker_dir));
    set_holiday_flags();

    DG52ED.stop_requested = 0;
    DG4E67.file_op_active = 0;
    DG4E67.word_4ec5 = 0xffff;

    load_archive_map();

    /*
     * What RESOURCE.CFG would have said, if it is not there.
     *
     * **NOT A TRANSCRIPTION for the two sound bytes. A deliberate deviation,
     * chosen by the project owner on 2026-09-04**, and the second of the two
     * in `reconstruct/src` - the other is `load_sound_bank`'s device 7.
     *
     * The original falls back to device 0 and module -2: the PC speaker, and
     * no digitised module. So does this, again - the port briefly fell back to
     * General Midi and `ASB:` instead, which was a deliberate deviation and
     * stopped meaning anything when the `GMD:` driver was removed.
     *
     * A RESOURCE.CFG decides in practice, and the game ships one.
     */
    cfg_first = 0;
    sound_module = -2;
    sound_device = 0;

    file = stdio_fopen(dg_ptr(dgroup, 0x00aa), dg_ptr(dgroup, 0x00b7));         /* "RESOURCE.CFG", "rb" */
    if (file != 0) {
        stdio_fread((volatile uint8_t *)&cfg_byte, 1, 1, file);
        cfg_first = ((int8_t)cfg_byte);
        stdio_fread((volatile uint8_t *)&cfg_byte, 1, 1, file);
        sound_device = ((int8_t)cfg_byte);
        stdio_fread((volatile uint8_t *)&cfg_byte, 1, 1, file);
        sound_module = ((int8_t)cfg_byte);
        stdio_fclose(file);
    }
    (void)cfg_first;    /* the original stores it and never reads it back */

    if (read_tim_cfg() == 0) {
        DG4E67.furthest_level = 1;
        DG4E67.master_level = 6;
    }

    DG4E67.password_puzzle = 0;
    DG4E67.score = 0;
    DG52BD.fill_colour = 3;
    DG52BD.word_52c9 = 0x0b;

    if (vm_init(0x0d, 0x80, 0x00ba) == 0) {     /* "vm.ovl" */
        stdio_printf(dg_ptr(dgroup, 0x1c30));                   /* "Unable to initialize vm." */
        stdio_exit(0);
    }

    DG3890.page_front_ptr = 0xa000;
    DG3890.page_back_ptr = 0xa820;
    vm_set_display_lines(0x1d6);                /* 470 - the Sierra logo */

    DG52ED.pal_tim_ptr.dword = (int32_t)load_palette(0x00c1);   /* "tim.pal"    */
    DG52BD.pal_sierra_ptr.dword = (int32_t)load_palette(0x00c9);   /* "sierra.pal" */
    {
        uint32_t black = load_palette(0x00d4);      /* "black.pal"  */

        DG52BD.pal_black_ptr.dword = (int32_t)black;
        set_palette_pointer((struct far_ptr){ (uint16_t)black, (uint16_t)(black >> 16) });
    }

    DG52BD.word_52df = load_font(0x00de);          /* "memofnt8.fnt" */
    set_font((int16_t)((uint16_t)DG52BD.word_52df));

    DG52ED.cursor_art_ptr = load_bitmap_list(0x00eb);          /* "mouse.bmp"   */
    DG52ED.panel_art_ptr = load_bitmaps(dg_ptr(dgroup, 0x00f5));          /* "cp.bmp"      */
    DG4E67.bmp_4ecb_ptr = load_bitmaps(dg_ptr(dgroup, 0x00fc));          /* "gp_bord.bmp" */

    install_keyboard(0);

    start_sound(sound_device, sound_module, 0, 0x0108);     /* "sx.ovl" */

    DG52ED.word_52f8 = open_file_record(dg_ptr(dgroup, 0x010f));   /* "tim.sx" */
    for (i = 1; i <= 0x14; i++)
        open_sound_file(DG52ED.word_52f8, (int16_t)i);

    /* A word table at DGROUP 0x116, indexed by what TIM.CFG put at 0x4ec1. */
    set_master_level_ok(DGU16((uint16_t)(0x116 + DG4E67.master_level * 2)));

    install_divide_trap();
    timer_install(0x0d);
    mouse_init();
    mouse_move_to(10, 10);
    timer_add_callback((struct far_ptr){ 0xa7ae, (uint16_t)(IMAGE_BASE >> 4) }, 4);

    select_cursor(0);
    erase_both_pages();
    mouse_set_speed(3);
    build_screen_regions();
    count_level_files();

    /*
     * Twenty eight-byte records off the near heap, chained through their first
     * word. 0x4e56 is the head; 0x4e58 is cleared with it and left alone.
     */
    DG4E4E.parts_queue_ptr = 0;
    DG4E4E.parts_free_ptr = 0;
    for (i = 0; i < 0x14; i++) {
        uint16_t p = heap_calloc_far(1, 8);

        QNODE(p).next = DG4E4E.parts_free_ptr;
        DG4E4E.parts_free_ptr = p;
    }

    /*
     * And 180 twenty-four-byte records from DOS, chained the same way but
     * through a *far* pointer - offset at 0x4e4e, segment at 0x4e50, and the
     * link in the first four bytes of each block. 0x4e52/0x4e54 are the second
     * head, cleared here and not filled.
     */
    DG4E4E.shapes_tail_ptr = 0;
    DG4E4E.shapes_ptr = 0;
    DG4E4E.shape_free.seg = 0;
    DG4E4E.shape_free.off = 0;
    for (i = 0; i < 0xb4; i++) {
        struct far_ptr block = dos_alloc_bytes(0x18, 0, 1).ptr;

        FARU16(block.seg, (uint16_t)(block.off + 2)) = DG4E4E.shape_free.seg;
        FARU16(block.seg, block.off) = DG4E4E.shape_free.off;
        DG4E4E.shape_free = block;
    }
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
    uint16_t bitmaps;                       /* [bp-0xc] */
    uint16_t gkc;                           /* [bp-0xe] */
    int16_t stage;                          /* [bp-4]  */
    int16_t budget;                         /* [bp-2]  */
    int16_t which;                          /* [bp-0xa] */
    int16_t frame;                          /* [bp-8]  */
    int16_t origin;                         /* the animation's left edge */
    int16_t running;                        /* [bp-6]  */
    const volatile struct intro_step *step;
    int16_t si;

    DG44EE.frame_budget = 0x2710;

    set_palette_pointer(DG52BD.pal_black_ptr.ptr);      /* black.pal */

    bitmaps = load_bitmaps(dg_ptr(dgroup, 0x254a));                         /* "sierra.bmp" */

    DG3890.page_back_ptr = 0xa000;
    DG3890.page_front_ptr = 0xa000;

    for (si = 0; si < 3; si++)
        present_frame(1);

    DG3890.page_back_ptr = (uint16_t)(DG3890.page_back_ptr + 0x12c);
    DG4E67.state = 0x8000;
    DG52BD.music_now = -1;

    stage = 0;
    step = &DG2370.step[0];

    for (;;) {
        if (stage == 0) {
            DG3890.page_dst_ptr = DG3890.page_front_ptr;
            clear_flag_2d44_thunk();
            load_screen(0x2555);                              /* "sierra.scr" */
            set_palette_pointer(DG52BD.pal_sierra_ptr.ptr);  /* sierra.pal */
            stage = 1;
            budget = (int16_t)(DG44EE.frame_budget + 0xff88);
            step = &DG2370.step[0];
        }

        /*
         * `jl` - the step runs while the counter is *under* the budget, and
         * the budget is set 0x78 below whatever the counter was. So nothing
         * moves until DGROUP 0x44ef counts down, which is the timer's doing:
         * this is the frame pacing, not a frame counter.
         */
        if (step->x != 0 && (int16_t)(DG44EE.frame_budget + 6) < budget) {
            DG3890.clip_enabled = 1;
            DG3890.clip_top = 0;
            DG3890.clip_left = 0;
            DG3890.clip_right = 0x27f;
            DG3890.clip_bottom = 0x1df;
            DG3890.fill_enabled = 1;
            DG3890.fill_colour = 0;
            DG3890.second_colour = 0;

            DG3890.page_dst_ptr = DG3890.page_back_ptr;
            fill_rect(0x1c0, 0x19f, 0xc0, 0x41);

            draw_bitmap(BMPP(DGU16((uint16_t)(bitmaps + 2 * step->bitmap))),
                        step->x, (int16_t)(step->y + 0x19f), 0);

            if (step->bitmap == 0)
                play_sound(0x14);

            step++;

            draw_bitmap(BMPP(DGU16((uint16_t)(bitmaps + 2 * step->bitmap))),
                        step->x, (int16_t)(step->y + 0x19f), 0);

            step++;

            DG3890.page_dst_ptr = DG3890.page_front_ptr;
            DG3890.page_src_ptr = DG3890.page_back_ptr;
            copy_rect_thunk(0x1c0, 0x1a9, 0xc0, 0x4b);

            budget = DG44EE.frame_budget;

            if (step->x == 0) {
                play_sound(19);
                stage = 4;
            }
        }

        regions_handle_pointer(DG4E67.regions_play_ptr);
        update_button_state();

        if (DG52ED.stop_requested != 0)
            game_teardown(1);

        if (stage == 4 || DG4E67.state != 0x8000)
            break;
    }

    free_bitmaps_thunk(BMPLIST(bitmaps));

    DG52BD.saved_clip_left = 0;
    DG52BD.saved_clip_top = 0;
    DG52BD.saved_clip_right = 0x27f;
    DG52BD.saved_clip_bottom = 0x18f;

    load_all_parts();

    gkc = load_bitmaps(dg_ptr(dgroup, 0x2560));                             /* "corners.bmp" */

    for (si = 0x37; si <= 0x39; si++)
        load_part_bitmap((uint16_t)si);

    set_palette_pointer(DG52BD.pal_black_ptr.ptr);      /* black.pal */

    DG3890.page_front_ptr = 0xa000;
    DG3890.page_back_ptr = 0xa820;

    for (si = 0; si < 3; si++)
        present_frame(1);

    DG3890.page_dst_ptr = 0xa000;
    vm_set_display_lines(0x18f);
    update_button_state();

    if (DG5768.button_left == 2 || DG5768.button_right == 2) {
        DG4E67.state = 2;
        which = 2;
    }

    frame = 0x3f6;
    origin = 0;

    if (DG4E67.state == 0x8000) {
        which = (int16_t)0x8000;
        DG4E67.state = 0x2000;
    } else {
        DG4E67.state = 2;
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

        DG4E67.origin_x = origin;
        DG4E67.origin_b_x = origin;
        DG4E67.origin_c_x = origin;
        DG4E67.origin_y = 0;
        DG4E67.origin_b_y = 0;
        DG4E67.origin_c_y = 0;

        clear_machine();
        set_clip_full_screen();

        DG3890.page_dst_ptr = DG3890.page_back_ptr;
        DG3890.fill_colour = (uint8_t)((uint8_t)DG52BD.fill_colour);
        DG3890.second_colour = (uint8_t)((uint8_t)DG52BD.fill_colour);
        DG3890.fill_enabled = 1;

        fill_rect(0, 0, 0x280, 0x190);

        step_and_draw_machine(1);
        draw_frame_corners(gkc);
        present_frame(1);

        DG3890.page_src_ptr = DG3890.page_front_ptr;
        DG3890.page_dst_ptr = DG3890.page_back_ptr;
        copy_rect_around_cursor(0, 0, 0x280, 0x190);

        select_music((int16_t)((uint16_t)which == 0x8000 ? 0x3e9 : frame));

        running = 1;

        while (running != 0) {
            if (((uint16_t)DG52BD.sound_request_01) != 0) DG52BD.sound_request_01 = 1;
            if (((uint16_t)DG52BD.sound_request_02) != 0) DG52BD.sound_request_02 = 1;
            if (((uint16_t)DG52BD.sound_request_09) != 0) DG52BD.sound_request_09 = 1;
            if (((uint16_t)DG52BD.sound_request_0c) != 0) DG52BD.sound_request_0c = 1;

            update_button_state();
            step_machine();
            mark_parts_in_dirty_rects();
            step_word_4e87();
            replay_shapes();

            step_and_draw_machine(0);
            draw_frame_corners(gkc);
            present_frame(1);

            if (DG4E67.machine_frames == 0)
                set_palette_pointer(DG52ED.pal_tim_ptr.ptr);  /* tim.pal */

            if (((uint16_t)DG52BD.sound_request_01) == 1) stop_music_or_effect(1);
            if (((uint16_t)DG52BD.sound_request_02) == 1) stop_music_or_effect(2);
            if (((uint16_t)DG52BD.sound_request_09) == 1) stop_music_or_effect(9);
            if (((uint16_t)DG52BD.sound_request_0c) == 1) stop_music_or_effect(0xc);

            shift_all_histories();

            if (DG5768.button_left == 2 || DG5768.button_right == 2) {
                DG4E67.state = 2;
                which = 2;
                running = 0;
            }

            DG4E67.machine_frames++;

            if ((uint16_t)which == 0x8000) {
                if ((int16_t)DG4E67.machine_frames > 0x110)
                    running = 0;
            } else if ((int16_t)DG4E67.machine_frames > 0x152) {
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

    DG4E67.icons_bmp_ptr = load_bitmaps(dg_ptr(dgroup, 0x2582));                   /* "icons.bmp" */
    DG4E67.state = 0x8000;

    copy_protect_screen(gkc);

    DG4E67.state = 2;

    set_palette_pointer(DG52BD.pal_black_ptr.ptr);      /* black.pal */
    present_frame(1);

    free_bitmaps_thunk(BMPLIST(gkc));

    stop_music_or_effect(0);
    show_cursor_again();

    DG3890.page_front_ptr = 0xa190;
    DG3890.page_back_ptr = 0xa8c0;
    DG3F78.screen_height = 0x16f;

    vm_set_display_lines(0x1bf);
    vm_set_line_compare(0x16f);

    for (si = 0; si < 3; si++)
        present_frame(1);

    DG52BD.saved_clip_left = 8;
    DG52BD.saved_clip_right = 0x237;
    DG52BD.saved_clip_top = 8;
    DG52BD.saved_clip_bottom = 0x167;
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
 * button at 0x248,0x158 calls `game_teardown`. Tab - scancode 0x0f out of
 * `bios_read_key` - walks a highlight around the grid and onto the button.
 *
 * **The copy this project was built from is cracked, in one byte, and this
 * routine is where.** The wait loop is entered on the wrong side and exits at
 * once: after `[bp-0x12]` is cleared the routine jumps to 0x0eddd, which
 * *sets* it to 1, and 0x0ede2 leaves when it is not zero. So the screen is
 * drawn and the routine returns without ever polling, and any answer passes.
 *
 * The arithmetic is exact rather than a reading of the listing. The bytes at
 * 0x0ec79 are `e9 61 01`, the next instruction is at 0x0ec7c, and
 * 0x0ec7c + 0x161 is 0x0eddd. The loop's **test** is at 0x0ede2 - `cmp
 * [bp-0x12], 0`, `jne` out at 0x0ede6, `jmp` back to the body at 0x0ede8 -
 * which is where a Borland `while` enters, and 0x0ede2 - 0x0ec7c is 0x166. So
 * the shipped byte is 0x61 where the compiler emitted 0x66:
 *
 *     e9 66 01   jmp 0x0ede2    the test, and the loop runs
 *     e9 61 01   jmp 0x0eddd    `done = 1`, and it does not
 *
 * One byte, landing on a real instruction boundary either way, turning the
 * check into a formality. No compiler emits a jump into the middle of a loop
 * body to set its own exit flag.
 *
 * **The default here is the binary's, crack and all**, so this file stays a
 * transcription: `goto check`, the loop never runs, and any answer passes -
 * which is what the shipped bytes do and what `out/TIM.img` still holds.
 *
 * **Both jumps are kept, under `TIM_COPY_PROTECTION`**, so the crack is
 * readable here rather than only in a commit message. Defining it takes the
 * jump to the loop's test instead and the screen waits for a real answer -
 * the same edit in C that 0x66 is in the binary, and a **deliberate
 * deviation** rather than a transcription. Pair it with `tools/uncrack.py`,
 * which takes the byte out of the image and the executable: an un-cracked port
 * against a cracked reference disagrees on this screen, and every screen
 * comparison that crosses it fails, so the two want moving together.
 *
 * The labels are guarded too, not just the `goto`. An unused label is a
 * `-Wall` warning, and the build is warning-clean.
 *
 * The distance between the two targets is worth keeping in view:
 * 0x0ede2 - 0x0eddd is 5, and `c7 46 ee 01 00` - `mov word [bp-0x12], 1` -
 * is five bytes. So the crack is exactly "take the length of the `done = 1`
 * instruction off the entry jump", landing it *on* that instruction instead of
 * after it. One byte, no relocation, no change of size.
 *
 * The loop body was transcribed all along, because it is there and has to be
 * right if it is ever reached; now it is reached.
 *
 * One earlier note is withdrawn. It said the original "does not run this
 * routine at all" while the screen is up, measured as zero addresses executed
 * in 0x0ea39..0x0edf0 from a snapshot taken there. That is what a routine that
 * has already drawn its screen and returned looks like - which is exactly what
 * the crack makes it do - so the measurement was of the patch, not of the
 * game.
 */
uint16_t copy_protect_screen(uint16_t bitmaps)
{
    uint8_t msg[80];              /* [bp-0x74], 0x50 bytes */
    uint8_t numbuf[22];   /* [bp-0x24] */
    int16_t answers[7];   /* [bp-0xe], three words */
    int16_t  page, done, slot, highlight, si;
    int16_t  x, y, part;

    DG3F78.screen_height = 0x18f;

    for (si = 0; si < 3; si++)
        answers[si] = -1;

    highlight = -1;
    slot      = 0;
    page      = (int16_t)(DG44EE.frame_budget & 0xf);

    set_clip_full_screen();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.fill_colour   = ((uint8_t)DG52BD.fill_colour);
    DG3890.second_colour   = ((uint8_t)DG52BD.fill_colour);
    DG3890.fill_enabled   = 1;

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
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x12]), 0x24c, 0x15e, 0);
    restore_cursor_following();

    int_to_string((int16_t)(page + 1), (volatile uint8_t *)numbuf, 10);
    string_copy((volatile uint8_t *)msg, dg_ptr(dgroup, 0x1c9e));   /* "Please select, in order, ... page " */
    string_concat((volatile uint8_t *)msg, (volatile uint8_t *)numbuf);
    string_concat((volatile uint8_t *)msg, dg_ptr(dgroup, 0x1cd7)); /* " of the user's manual." */
    draw_scroll_text((volatile uint8_t *)msg, 0x40, 0x106, 0x200);

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
        draw_bitmap_centred(BMPSET(DG4E67.icons_bmp_ptr).bmp[part],
                            x, y, 0x40, 0x30);
        restore_cursor_following();
    }

    select_music((int16_t)(page + 0x3e9));
    present_frame(1);

    DG3890.page_src_ptr = DG3890.page_front_ptr;
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    copy_rect_around_cursor(0, 0, 0x280, 0x190);
    set_palette_pointer(DG52ED.pal_tim_ptr.ptr);
    show_cursor_again();

    done = 0;
#ifdef TIM_COPY_PROTECTION
    goto test;                  /* e9 66 01: jmp 0x0ede2, the loop's test */
#else
    goto check;                 /* e9 61 01: jmp 0x0eddd, `done = 1` */
#endif

    for (;;) {
        update_button_state();

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);
        if (((uint8_t)DG52ED.last_key) == 0x0f) {          /* Tab walks the highlight */
            highlight++;
            if (highlight == 0x21)
                highlight = 0;
            if (highlight == 0x20)
                move_pointer_to(0x268, 0x188);    /* the button */
            else
                move_pointer_to((uint16_t)(((highlight % 8) << 6) + 0x50),
                          (uint16_t)((highlight / 8) * 0x30 + 0x30));
        }

        select_cursor((DG5768.pointer_x >= 0x248 && DG5768.pointer_y >= 0x158)
                      ? 0x15 : 0);

        if (((int16_t)DG5768.button_left) == 2) {            /* the frame of a click */
            if (DG5768.pointer_x >= 0x40 && DG5768.pointer_x < 0x240
                && DG5768.pointer_y >= 0x20 && DG5768.pointer_y < 0xe0) {
                part = (int16_t)((DG5768.pointer_x - 0x40) / 0x40
                                 + ((DG5768.pointer_y - 0x20) / 0x30) * 8);
                if (part > 0x13)
                    part++;
                if (part == 0x1e)
                    part = 0x23;
                if (part == 0x20)
                    part = 0x24;

                answers[slot] = part;
                draw_answer_slot(BMPSET(DG4E67.icons_bmp_ptr).bmp[part],
                                 (uint16_t)slot);
                slot++;
                if (slot == 3)
                    slot = 0;
            }

            if (DG5768.pointer_x >= 0x248 && DG5768.pointer_y >= 0x158)
                game_teardown(1);
        }

        present_frame(1);

        if (DG16((uint16_t)(0x24ea + 2 * page)) == answers[0]
            && DG16((uint16_t)(0x250a + 2 * page)) == answers[1]
            && DG16((uint16_t)(0x252a + 2 * page)) == answers[2])
#ifndef TIM_COPY_PROTECTION
check:
#endif
            done = 1;

#ifdef TIM_COPY_PROTECTION
test:
#endif
        if (done != 0)
            break;
    }
    return 0;
}

/*
 * 0x0edf1
 *
 * **Draw the part you just picked into one of the three answer slots** on the
 * copy-protection screen. `bmp` is the part's icon out of the table
 * `copy_protect_screen` loaded into DGROUP 0x4ec7, and `slot` is 0, 1 or 2.
 *
 * The slot's box is 0x40 by 0x30 at y 0x12c, and its x is `slot * 0x60 + 0xc0`
 * - so the three sit 0x60 apart starting at 0xc0, which is 0x20 wider than the
 * boxes and leaves the gap between them.
 *
 * The panel is drawn first and the icon centred into it afterwards, so a part
 * whose picture is smaller than the box is not left with the previous pick's
 * pixels around it. `clear_flag_2d44_thunk` and `restore_cursor_following`
 * bracket the drawing the way they do everywhere the cursor might be over what
 * is being painted.
 *
 * Then the page pointers are put back - 0x38a6 from 0x38a4 and 0x38a8 from
 * 0x38a2 - and the whole screen is copied around the cursor, because the frame
 * just presented was drawn on the other page.
 *
 * **Unreachable until 2026-09-06.** It is called only from the click inside the
 * grid, and the crack removed from `copy_protect_screen` returned before the
 * screen ever polled, so no click ever arrived. Restoring that one byte is what
 * made this reachable, and it aborted on the first click.
 */
void draw_answer_slot(uint16_t bmp, uint16_t slot)
{
    int16_t x;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    x = (int16_t)(slot * 0x60 + 0xc0);

    draw_panel(x, 0x12c, 0x40, 0x30);
    clear_flag_2d44_thunk();
    draw_bitmap_centred(bmp, x, 0x12c, 0x40, 0x30);
    restore_cursor_following();
    present_frame(1);

    DG3890.page_src_ptr = DG3890.page_front_ptr;
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    copy_rect_around_cursor(0, 0, 0x280, 0x190);
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

    draw_bitmap(BMPP(DGU16(rec)), 0, 0, 0);
    draw_bitmap(BMPP(DGU16((uint16_t)(rec + 2))), 0x262, 0, 0);
    draw_bitmap(BMPP(DGU16((uint16_t)(rec + 4))), 0, 0x175, 0);
    draw_bitmap(BMPP(DGU16((uint16_t)(rec + 6))), 0x262, 0x175, 0);

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

    while (DG4E67.playing != 0) {
        game_round();

        if (((int16_t)DG4E67.state) == 1) {
            DG4E67.playing = 0;
        } else {
            DG4E67.round_number = (int16_t)(DG4E67.round_number + 1);
            if (DG4E67.round_number > DG4E67.furthest_level) {
                DG4E67.furthest_level = DG4E67.round_number;
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
    bar = load_bitmaps(dg_ptr(dgroup, 0x25e8));                 /* "score1.bmp" */

    DG3890.page_dst_ptr = 0xa000;
    DG3890.fill_colour = 0;
    DG3890.second_colour = 0;
    DG3890.fill_enabled = 1;

    fill_rect(0, 0, 0x280, 0x50);

    draw_bitmap(BMPP(DGU16(bar)), 3, 0, 0);
    draw_bitmap(BMPP(DGU16((uint16_t)(bar + 2))), 0x107, 0, 0);
    draw_bitmap(BMPP(DGU16((uint16_t)(bar + 4))), 0x1bb, 0, 0);

    free_bitmaps_thunk(BMPLIST(bar));

    clear_flag_2d44_thunk();
    DG4E67.menu_bmp_ptr = load_bitmaps(dg_ptr(dgroup, 0x25f3));       /* "gp_menu.bmp" */
    DG4E67.score2_bmp_ptr = load_bitmaps(dg_ptr(dgroup, 0x25ff));       /* "score2.bmp"  */

    DG4E67.counter = 0;
    DG4E67.round_number = 1;
    DG4E67.playing = 1;
    DG4E67.round_kind = 0;
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
    DG4E67.origin_y = -8;
    DG4E67.origin_x = -8;
    DG4E67.origin_b_y = -8;
    DG4E67.origin_b_x = -8;
    DG4E67.origin_c_y = -8;
    DG4E67.origin_c_x = -8;
    DG4E67.word_4ebb = 0;

    heap_check_or_hang();

    if (DG4E67.round_kind != 0) {
        build_part_list();
        reset_machine();
    } else {
        load_level(((uint16_t)DG4E67.round_number));
    }

    DG50AF.extent_x = -8;
    DG50AF.extent_y = -8;

    start_counters();
    reset_input_state();

    DG4E67.state = 2;
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
 * anything. And 0x200 alone gets `finish_level` called on the way out, which
 * is the one asymmetry in it.
 *
 * A `while` again rather than a `do`: the entry jump at 0x0effd goes to the
 * test. With the state at 2 the test passes, so the loop always runs at least
 * once in practice - but it is written as a test-first loop and is transcribed
 * as one.
 */
void game_round(void)
{
    round_setup();

    while (DG4E67.state != 0x200 && DG4E67.state != 1) {
        heap_check_or_hang();

        if (DG4E67.state == 2)
            game_screen();
        else if (DG4E67.state == 0x2000)
            run_machine_loop();
        else
            game_screen_loop();
    }

    if (DG4E67.state == 0x200)
        finish_level();

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
    uint8_t name[14];
    uint8_t digits[8];

    string_copy((volatile uint8_t *)name, dg_ptr(dgroup, 0x2876));
    int_to_string((int16_t)number, (volatile uint8_t *)digits, 10);
    string_concat((volatile uint8_t *)name, (volatile uint8_t *)digits);
    string_concat((volatile uint8_t *)name, dg_ptr(dgroup, 0x2878));

    DG546C.is_level = 1;
    read_level((volatile uint8_t *)name);
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
    uint8_t title[120];
    uint8_t digits[8];

    if (DG4E67.round_kind != 0) {
        string_copy((volatile uint8_t *)title, dg_ptr(dgroup, 0x21d4));             /* "FREEFORM MODE" */
    } else {
        string_copy((volatile uint8_t *)title, dg_ptr(dgroup, 0x21e2));             /* "PUZZLE " */
        int_to_string(DG4E67.round_number, (volatile uint8_t *)digits, 10);
        string_concat((volatile uint8_t *)title, (volatile uint8_t *)digits);
        string_concat((volatile uint8_t *)title, dg_ptr(dgroup, 0x2837));
        string_concat((volatile uint8_t *)title, dg_ptr(dgroup, 0x4ecf));           /* the level's own title */
    }

    set_clip_play_area();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    draw_title_bar(0x20, 0x20, 0x220, 0x158, 1);
    fill_panel_area(0x110, 0x48, 0x100, 0xa0, ((uint16_t)DG52BD.fill_colour));

    draw_scroll_text((volatile uint8_t *)title, 0x3c, 0x27, 0x1bc);
    draw_panel(0x110, 0xff, 0x100, 0x4c);

    if (DG4E67.round_kind != 0)
        draw_wrapped_text(0x22c0, 0x114, 0x104, 0xf8, 0x44);
    else
        draw_wrapped_text(0x4f1f, 0x114, 0x104, 0xf8, 0x44);

    paint_panel_frame_rest();
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
    dg_off_t set = DG4E67.bmp_4ecb_ptr;
    int16_t  x, y;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.clip_enabled = 0;
    DG3890.fill_enabled = 1;
    DG3890.fill_colour   = 0;
    DG3890.second_colour = 0;

    clear_flag_2d44_thunk();

    if (filled != 0) {
        fill_rect((int16_t)(x1 - 0x0c), (int16_t)(y1 + 0x0c),
                  (int16_t)(x2 - x1), (int16_t)(y2 - y1));
        draw_bitmap(BMPP(BMPSET(set).bmp[0x25]),
                    (int16_t)(x1 - 0x0f), (int16_t)(y1 + 7), 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x26]),
                    (int16_t)(x1 - 0x0f), (int16_t)(y2 - 9), 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x27]),
                    (int16_t)(x2 - 0x20), (int16_t)(y2 - 9), 0);
    }

    DG3890.clip_left    = x1;
    DG3890.clip_right   = x2;
    DG3890.clip_top     = y1;
    DG3890.clip_bottom  = y2;
    DG3890.clip_enabled = 1;

    for (y = y1; y < y2; y = (int16_t)(y + 0x40))
        for (x = x1; x < x2; x = (int16_t)(x + 0x80))
            draw_bitmap(BMPP(BMPSET(set).bmp[0x2a]), x, y, 0);

    if (DG4E67.state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    DG3890.clip_enabled = 0;

    for (x = x1; x < x2; x = (int16_t)(x + 8)) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x12]), x, (int16_t)(y1 - 4), 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x13]), x, y2, 0);
    }

    for (y = y1; y < y2; y = (int16_t)(y + 8)) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x10]), (int16_t)(x1 - 4), y, 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x11]), x2, y, 0);
    }

    draw_bitmap(BMPP(BMPSET(set).bmp[0xc]),
                (int16_t)(x1 - 7), (int16_t)(y1 - 7), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xd]),
                (int16_t)(x2 - 0x11), (int16_t)(y1 - 7), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xe]),
                (int16_t)(x1 - 7), (int16_t)(y2 - 0x11), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xf]),
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
    dg_off_t set = DG4E67.bmp_4ecb_ptr;
    int16_t  x2  = (int16_t)(x + w);
    int16_t  y2  = (int16_t)(y + h);
    int16_t  n;

    clear_flag_2d44_thunk();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    DG3890.fill_colour = (uint8_t)colour;
    DG3890.second_colour = (uint8_t)colour;

    fill_rect(x, y, w, h);

    for (n = x; n < x2; n = (int16_t)(n + 8)) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x1a]), n, (int16_t)(y - 8), 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x1b]), n, y2, 0);
    }

    for (n = y; n < y2; n = (int16_t)(n + 8)) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x18]), (int16_t)(x - 8), n, 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x19]), x2, n, 0);
    }

    draw_bitmap(BMPP(BMPSET(set).bmp[0x14]),
                (int16_t)(x - 8), (int16_t)(y - 8), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x15]),
                (int16_t)(x2 - 8), (int16_t)(y - 8), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x16]),
                (int16_t)(x - 8), (int16_t)(y2 - 5), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x17]),
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

    DG3890.unknown_02 = 1;                        /* transparent */
    line_height = font_line_height(0);

    wrap_text_to_box(str, w, h, line_height);

    left = (int16_t)(x + (w - DG568F.text_width - 1) / 2);
    top  = (int16_t)(y + (h - DG568F.text_height - 1) / 2 + 1);

    DG3890.clip_left   = left;
    DG3890.clip_right  = (int16_t)(left + w);
    DG3890.clip_top    = top;
    DG3890.clip_bottom = (int16_t)(top + h);

    entry   = dg_off(dgroup, &DG56A6.line[0]);
    left_at = DG568F.line_count;

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

        DG3890.unknown_00 = 0x0f;
        draw_string(dg_ptr(dgroup, start), (int16_t)(left - 1), (int16_t)(top + 1));

        DG3890.unknown_00 = 5;
        draw_string(dg_ptr(dgroup, start), left, top);

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
    uint8_t space[2];            /* [bp-0xc], a two-byte " " */
    int16_t o_len[3];   /* [bp-0xa] */
    int16_t o_wide[2];   /* [bp-4]   */
    uint16_t at     = str;
    int16_t  used   = 0;         /* height used so far */
    int16_t  run    = 0;         /* width on the current line */
    int16_t  space_w;
    int16_t  cap    = (int16_t)(line_height * 7);

    if (h > cap)
        h = cap;

    DG568F.line_count = 0;
    DG568F.text_height  = 0;
    DG568F.text_width  = 0;

    if (DG8(at) != 0) {
        DG56A6.line[(uint16_t)DG568F.line_count] = at;
        DG568F.line_count++;
    }

    (*space)     = ' ';
    space[1] = 0;
    space_w = (int16_t)text_width_thunk((volatile uint8_t *)space);

    while (DG8(at) != 0 && (int16_t)(used + line_height) < h) {
        int16_t word_w, word_len;

        measure_word(dg_ptr(dgroup, at), (volatile uint8_t *)o_wide,
                     (volatile uint8_t *)o_len);
        word_w   = o_wide[0];
        word_len = o_len[0];

        if ((run != 0 || used == 0) && (int16_t)(run + word_w) >= w) {
            run  = 0;
            used = (int16_t)(used + line_height);
            DG56A6.line[(uint16_t)DG568F.line_count] = at;
            DG568F.line_count++;
            if ((int16_t)(used + line_height) >= h)
                break;
        }

        at  = (uint16_t)(at + word_len);
        run = (int16_t)(run + word_w);
        if (run > DG568F.text_width)
            DG568F.text_width = run;
        if (DG568F.text_width > w)
            DG568F.text_width = w;

        while (DG8(at) != 0 && DG8(at) <= ' '
               && (int16_t)(used + line_height) < h) {
            if (DG8(at) == 0x0d) {
                run  = 0;
                used = (int16_t)(used + line_height);
                DG56A6.line[(uint16_t)DG568F.line_count] = (uint16_t)(at + 1);
                DG568F.line_count++;
            } else if (DG8(at) == ' ') {
                run = (int16_t)(run + space_w);
            }
            at++;
        }
    }

    DG568F.text_height = used;

    if (run == 0 && ((uint16_t)DG568F.line_count) != 0)
        DG568F.line_count--;
    else
        DG568F.text_height = (int16_t)(DG568F.text_height + line_height);

    DG56A6.line[(uint16_t)DG568F.line_count] = at;
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
void measure_word(volatile uint8_t * str, volatile uint8_t * out_width, volatile uint8_t * out_length)
{
    volatile uint8_t * at  = str;
    int16_t  len = 0;
    uint8_t  saved;

    while (*at > ' ') {
        at++;
        len++;
    }

    saved   = *at;
    *at = 0;

    dg_wr16(out_width, (int16_t)text_width(str));
    dg_wr16(out_length, len);

    *at = saved;
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

    DG3890.clip_enabled = 1;
    set_clip_for_mode();

    extent = (DG50AF.extent_y > DG50AF.extent_x) ? DG50AF.extent_y : DG50AF.extent_x;
    extent = (int16_t)(extent + 0x230);

    scale = (int16_t)long_divide(0x40000, (int32_t)extent);

    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    rec = (uint16_t)pick_by_flag(0x3000);
    while (rec != 0) {
        link_record_into_buckets(PARTP(rec));
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x10]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x12]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x1f]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x29]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x21]),
                0x96, 0x8c, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x1d]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x23]),
                0xc8, 0x8c, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x1d]),
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
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[frame + 0x1b]),
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

    left  = (DG4E67.state == 0x4000) ? 0x26 : 0x25;
    right = (DG4E67.state == 0x2000) ? 0x28 : 0x27;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    for (si = 0x84; si < 0xb4; si = (int16_t)(si + 8))
        for (di = 0x5f; di <= 0x77; di = (int16_t)(di + 8))
            draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x2b]), si, di, 0);

    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[left]),  0x58, 0x5d, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[right]), 0x58, 0x6f, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x14]),      0x6e, 0x60, 0);

    y = 0x69;
    for (si = 1; si <= ((int16_t)DG4E67.master_level); si++) {
        draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[si + 0x14]),
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

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x7]), 0x41, 0xc8, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x9]), 0x3d, 0xe5, 0);

    at = (int16_t)long_divide(
             mul16x16(DG50AF.air, 0xa0), 0x200);

    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x6]),
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

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x8]), 0x41, 0x114, 0);
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x9]), 0x3d, 0x131, 0);

    at = (int16_t)long_divide(
             mul16x16(DG50AF.gravity, 0xa0), 0x80);

    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[0x6]),
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
    dg_off_t set;

    wait_cursor();
    set_clip_play_area();

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.fill_colour = ((uint8_t)DG52BD.fill_colour);
    DG3890.second_colour = ((uint8_t)DG52BD.fill_colour);
    DG3890.fill_enabled = 1;

    clear_flag_2d44_thunk();
    fill_rect(8, 8, 0x230, 0x160);

    draw_machine_thunk();
    paint_panel_frame();

    draw_panel(0x2c, 0x42, 0xd0, 0x109);

    paint_panel_a(0);
    paint_panel_b(0);
    paint_panel_c(0);
    paint_panel_d(0);

    if (DG4E67.round_kind != 0) {
        paint_panel_free_a(0);
        paint_panel_free_b(0);
    } else {
        paint_panel_level(0);
    }

    paint_panel_e();
    paint_panel_f();
    paint_panel_g();

    clear_flag_2d44_thunk();
    set = DG52ED.panel_art_ptr;
    draw_bitmap(BMPP(BMPSET(set).bmp[0x3]), 0x53, 0x42, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x5]), 0x64, 0xb2, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x4]), 0x5b, 0xfe, 0);
    restore_cursor_following();

    select_music(DG50AF.tune);

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
 * first, which is a `setvbuf` and nothing to do with the level's contents.
 *
 * **Two callers, one routine.** `load_level` sets 0x5472 and asks for
 * "l<n>.lev"; `load_animation` (0x12915) clears it and asks for an animation,
 * which is why the two strings above are read on one path and not the other.
 * This was transcribed twice - once under each caller's name - and the copies
 * drifted: the second read only 0x4ecf where the original reads 0x4f1f as
 * well, and answered a fabricated 0. It never bit, because the caller that
 * skipped the string is the caller that clears 0x5472 and so never reaches
 * it. There is one `sub sp,0x216` in the image and there is one of these.
 */
uint16_t read_level(volatile uint8_t * name)
{
    /*
     * **One slot has to be the guest's.** `buf` is the 0x210-byte stdio
     * buffer, and `stdio_setbuf_for` files its address into the file record's
     * `read_ptr` at +0x0a - a *guest word*, which the layer then steps as a
     * cursor, compares against `(uint16_t)(file + 5)` to tell a set buffer
     * from the record's own, and frees as a heap handle. Sixteen bits is the
     * whole of it and the port cannot promise a C object an address that fits.
     * The six count bytes below it are a C array, so the reservation is only
     * for the buffer.
     */
    uint16_t fp  = dg_alloca(0x216);
    uint16_t buf = fp;

    /* [bp-6], [bp-4], [bp-2]: three words `game_fread_far` fills, and nothing
       outside this routine ever sees their address. */
    _Alignas(2) uint8_t counts[6];
    uint16_t file;
    uint16_t r;
    int16_t  n_machine, n_moving, n_given;

    file = game_fopen(name, dg_ptr(dgroup, 0x2870));
    if (file == 0) {
        DG50D3.bin_list_ptr = 0x50d7;
        dg_free(0x216);
        return 0;   /* AX is the failed `game_fopen`'s, which is 0 */
    }

    stdio_setbuf_for(file, buf);
    game_fread_far(file, dg_ptr(dgroup, 0x5476));

    if (DG546C.version_out == 0xaced) {
        game_fread_far(file, dg_ptr(dgroup, 0x5474));

        if (DG546C.is_level != 0) {
            game_fread_string(file, dg_ptr(dgroup, 0x4ecf));
            game_fread_string(file, dg_ptr(dgroup, 0x4f1f));
            game_fread_far(file, dg_ptr(dgroup, 0x50af));
            game_fread_far(file, dg_ptr(dgroup, 0x50b1));
        }

        game_fread_far(file, dg_ptr(dgroup, 0x50b3));
        game_fread_far(file, dg_ptr(dgroup, 0x50b5));
        recompute_kind_physics();

        if (DG546C.is_level != 0) {
            game_fread_far(file, dg_ptr(dgroup, 0x50b7));
            game_fread_far(file, dg_ptr(dgroup, 0x50b9));
        }

        game_fread_far(file, dg_ptr(dgroup, 0x50bb));

        game_fread_far(file, counts + 4);
        game_fread_far(file, counts + 2);
        game_fread_far(file, counts);
        n_machine = dg_rd16(counts + 4);
        n_moving  = dg_rd16(counts + 2);
        n_given   = dg_rd16(counts);

        DG546C.record_count = 0;
        alloc_part_table((int16_t)(n_machine + n_moving + n_given));

        read_list(file, 0x521b, n_machine);
        read_list(file, 0x5179, n_moving);
        if (DG546C.is_level != 0)
            read_list(file, 0x50d7, n_given);

        dos_free_far(DG546C.table);
    }

    r = game_fclose(file);
    DG50D3.bin_list_ptr = 0x50d7;

    /* The epilogue is `mov [0x50d3],0x50d7 / pop si / mov sp,bp / pop bp /
       retf` - nothing touches AX after the close, so the close's answer is
       the routine's. */
    dg_free(0x216);
    return r;
}

/*
 * 0x0f0b0
 *
 * **The SELECT PUZZLE screen**, and what "leave freeform mode" puts up before
 * going back to the puzzles: a list of them, and a field for the password that
 * unlocks one you have not reached.
 *
 * It answers **whether the puzzle changed** - 1 when the score it leaves at
 * 0x4ebd differs from the one it arrived with - so the caller knows whether to
 * load a level.
 *
 * **It saves the score at 0x4eaf/0x4ead on the way in** and restores it if the
 * player presses Escape, because entering a score code overwrites it and Escape
 * has to mean "as you were".
 *
 * The furthest level reached at 0x4eb7 is the gate: a row beyond it puts up
 * NEED PASSWORD and does nothing else. A password that is found unlocks its
 * level; a score code that verifies sets the score with it, and one that does
 * not sets the score to **zero** rather than refusing - the message says so.
 * Passing 0x4eb7 writes `tim.cfg` on the spot, so a level unlocked by password
 * is still unlocked next time the game is run.
 *
 * Both scroll arrows move a **page** of 0x15 rather than a row, and both set
 * `di` to 4, which is the auto-repeat delay: four passes of the loop have to go
 * by before the arrow can move again. That is the same counter the mode word
 * uses to say the arrow is held.
 *
 * The five repaint counters and the `was` local are `pick_file`'s, and so is
 * the rule that a full repaint suppresses the partial ones.
 */
uint16_t sub_0f0b0(void)
{
    int32_t  saved;                     /* [bp-0x14], [bp-0x12] */
    int16_t  page;                      /* [bp-0x10] */
    int16_t  level;                     /* [bp-4]    */
    int16_t  row;                       /* [bp-2]    */
    int16_t  full     = 1;              /* [bp-8]    */
    int16_t  rp_up    = 0;              /* [bp-0xa]  */
    int16_t  rp_down  = 0;              /* [bp-0xc]  */
    int16_t  rp_pass  = 0;              /* [bp-0xe]  */
    uint16_t was      = 0x8000;         /* [bp-6]    */
    int16_t  repaint  = 0;              /* si */
    int16_t  hold     = 0;              /* di */

    saved = DG4E67.counter;

    DG53FC.selected_level = ((uint16_t)DG4E67.round_number);
    page = (int16_t)puzzle_page_of_score();
    DG53FC.word_542c = page;

    /*
     * The screen **opens the current level** before it draws anything: if the
     * player is on a puzzle further than 0x4eb7 says has been reached, 0x4eb7
     * catches up. So arriving here is itself enough to unlock the row you are
     * standing on, and the list's colours are right on the first paint.
     */
    if (DG4E67.round_number > DG4E67.furthest_level)
        DG4E67.furthest_level = DG4E67.round_number;

    DG4E67.state = 0x8000;

    for (;;) {
        update_button_state();
        DG52ED.last_key = (uint8_t)bios_read_key();

        if (((uint8_t)DG52ED.last_key) == 9 && DG4E67.state != 0x800)
            puzzle_tab();

        if ((((uint8_t)DG52ED.last_key) == 0x0d || ((uint8_t)DG52ED.last_key) == 0x20
             || ((uint8_t)DG52ED.last_key) == 0x1b)
            && DG4E67.state == 0x800)
            DG5768.button_left = 0;

        regions_handle_pointer(DG4E67.regions_a_ptr);

        if (((uint8_t)DG52ED.last_key) == 0x1b) {
            /*
             * Escape: put the score back, restart the counters, reset the clip,
             * and leave with the mode the loop's tail ends on.
             */
            DG4E67.counter = saved;
            start_counters();
            set_clip_play_area();
            DG53FC.selected_level = ((uint16_t)DG4E67.round_number);
            DG4E67.state = 0x400;
            goto tail;
        }

        /*
         * The password field, entered while it has focus or had it last pass -
         * `pick_file`'s trick, and for the same reason.
         */
        if (DG4E67.state == 0x800 || was == 0x800) {
            if ((((uint8_t)DG52ED.last_key) == 0x0d || DG4E67.state != 0x800)
                && was == 0x800) {
                update_button_state();

                level = (int16_t)password_to_level(0x542e);

                if (level == -1) {
                    show_message_box(0x2116 /* "BAD PASSWORD" */, 0x2123);
                    DG4E67.state = 0x8000;
                    full = 1;
                } else {
                    int32_t score = score_code_to_score(0x542e);

                    DG4E67.counter = score;

                    /* -1 in both halves is -1 in the whole. */
                    if (DG4E67.counter == -1) {
                        DG4E67.counter = 0;
                        show_message_box(0x2141 /* "SCORE CODE INVALID" */,
                                         0x2154);
                        full = 1;
                    }

                    DG53FC.selected_level = level;

                    if (DG53FC.selected_level > DG4E67.level_count)
                        DG53FC.selected_level = DG4E67.level_count;

                    repaint = 1;
                    start_counters();
                    set_clip_play_area();

                    page = (int16_t)puzzle_page_of_score();
                    if (page != DG53FC.word_542c) {
                        DG53FC.word_542c = page;
                        repaint = 1;
                    }

                    if (level > DG4E67.furthest_level) {
                        DG4E67.furthest_level = level;
                        sub_12bed();            /* write tim.cfg */
                        repaint = 1;
                    }

                    if (DG4E67.state == 0x800)
                        DG4E67.state = 0x8000;
                }
            } else {
                if (was == 0x800)
                    picker_type(((uint8_t)DG52ED.last_key), 0x542e, 0x19);
            }

            rp_pass = 2;
        }

    tail:
        if (hold != 0)
            hold--;

        switch (DG4E67.state) {
        case 0x2000:                    /* the up arrow: a page back */
            if (hold == 0) {
                if (DG5768.button_left != 1 && DG5768.button_left != 2) {
                    DG4E67.state = 0x8000;
                } else if (DG53FC.word_542c > 1) {
                    DG53FC.word_542c = (int16_t)(DG53FC.word_542c - 0x15);
                    if (DG53FC.word_542c < 1)
                        DG53FC.word_542c = 1;
                    repaint = 1;
                    hold    = 4;
                }
            }
            rp_up = 2;
            break;

        case 0x1000:                    /* the down arrow: a page on */
            if (hold == 0) {
                if (DG5768.button_left != 1 && DG5768.button_left != 2) {
                    DG4E67.state = 0x8000;
                } else if ((int16_t)(DG53FC.word_542c + 0x15) <= DG4E67.level_count) {
                    DG53FC.word_542c = (int16_t)(DG53FC.word_542c + 0x15);
                    repaint = 1;
                    hold    = 4;
                }
            }
            rp_down = 2;
            break;

        case 0x4000:                    /* a click in the list */
            row = (int16_t)((int16_t)(DG5768.pointer_y - 0x4c) / 10
                            + DG53FC.word_542c);

            if (row <= DG4E67.level_count) {
                if (row > DG4E67.furthest_level) {
                    show_message_box(0x20c4 /* "NEED PASSWORD" */, 0x20d2);
                    repaint = 1;
                } else if (row != DG53FC.selected_level) {
                    DG53FC.selected_level = row;

                    /*
                     * Choosing a puzzle *earlier* than the one being played
                     * zeroes the score, because the score belongs to the run
                     * that got this far.
                     */
                    if (DG53FC.selected_level < DG4E67.round_number) {
                        DG4E67.counter = 0;
                        start_counters();
                        set_clip_play_area();
                    }

                    repaint = 1;
                }
            }

            DG4E67.state = 0x8000;
            break;

        default:
            break;
        }

        was = DG4E67.state;

        if (full != 0) {
            wait_cursor();
            puzzle_repaint();
            restore_cursor();
            full--;
        } else {
            if (rp_up != 0) {
                puzzle_draw_up();
                present_back_page();
                rp_up--;
            }
            if (rp_down != 0) {
                puzzle_draw_down();
                present_back_page();
                rp_down--;
            }
            if (repaint != 0)
                puzzle_draw_list(DG53FC.word_542c, DG53FC.selected_level);
            if (rp_pass != 0) {
                puzzle_draw_password(0x542e);
                rp_pass--;
            }
        }

        if (repaint != 0) {
            present_back_page();
            repaint--;
        } else {
            present_frame(1);
        }

        if (DG4E67.state == 0x400)
            break;
    }

    puzzle_draw_ok(1);
    present_back_page();

    if (((uint16_t)DG53FC.selected_level) != ((uint16_t)DG4E67.round_number)) {
        DG4E67.round_number = ((uint16_t)DG53FC.selected_level);
        return 1;
    }

    return 0;
}

/*
 * 0x0f468
 *
 * **Tab on the puzzle screen**, the third of these and the same trick: warp the
 * pointer. Five stops, cursor at DGROUP 0x260a, x at 0x260c and y at 0x2616 -
 * so 0x260c holds exactly five words and 0x2616 begins where it ends.
 */
void puzzle_tab(void)
{
    DG260A.word_260a++;

    if (DG260A.word_260a == 5)
        DG260A.word_260a = 0;

    move_pointer_to(DG16((uint16_t)(0x260c + 2 * DG260A.word_260a)),
                    DG16((uint16_t)(0x2616 + 2 * DG260A.word_260a)));
}

/*
 * 0x0f499
 *
 * **Which page a score is on.** Pages hold 0x15 puzzles and are numbered from
 * **1**, so this walks 1, 0x16, 0x2b... until the page's last puzzle - its
 * first plus 0x14 - reaches the score at DGROUP 0x542a, and answers the first.
 *
 * It counts rather than divides, which is why the page numbers are one-based
 * without any correction: the arithmetic never has to be shifted.
 */
uint16_t puzzle_page_of_score(void)
{
    int16_t page = 1;

    while ((int16_t)(page + 0x14) < DG53FC.selected_level)
        page = (int16_t)(page + 0x15);

    return (uint16_t)page;
}

/*
 * 0x0f57e
 *
 * The puzzle list's **up arrow**, and `picker_draw_up`'s twin in a different
 * screen: the same art at +0x4a of the set, the pressed one chosen by reading
 * 0x4e6b back - 0x2000 here - and only the position differs.
 */
void puzzle_draw_up(void)
{
    int16_t pressed = (DG4E67.state == 0x2000) ? 1 : 0;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[pressed + 0x25]),
                0x1d4, 0x46, 0);
    restore_cursor_following();
}

/*
 * 0x0f5c4
 *
 * The puzzle list's **down arrow**: art at +0x4e, mode 0x1000, and 0xca pixels
 * below its twin.
 */
void puzzle_draw_down(void)
{
    int16_t pressed = (DG4E67.state == 0x1000) ? 1 : 0;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[pressed + 0x27]),
                0x1d4, 0x110, 0);
    restore_cursor_following();
}

/*
 * 0x0f60a
 *
 * The puzzle screen's **OK button**, and unlike the two arrows it is *told*
 * whether it is pressed rather than reading the mode - because the one caller
 * that draws it pressed does so on the way out of the loop, after the mode has
 * already been changed to the one that ends it.
 */
void puzzle_draw_ok(uint16_t pressed)
{
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[pressed + 0x10]),
                0x200, 0x12e, 0);
    restore_cursor_following();
}

/*
 * 0x0f4b5
 *
 * **The puzzle screen's whole surface**, the counterpart of `picker_repaint`:
 * the title bar, two headings, three sunken wells, and then the same five
 * routines the loop calls for its partial redraws.
 *
 * The three wells are all placed round what goes in them: two 0x20 squares at
 * x 0x1cc for the arrows drawn at 0x1d4, and a 0x28 square at (0x1f0, 0x12c)
 * for the OK button at (0x200, 0x12e). The list has no well - it is drawn onto
 * the title bar's own surface, and `puzzle_draw_list` fills its rectangle
 * itself before writing the rows.
 *
 * The OK button is drawn with `0`, unpressed, because this is the paint that
 * puts the screen up rather than the one that answers a click.
 */
void puzzle_repaint(void)
{
    draw_title_bar(0x20, 0x20, 0x220, 0x158, 0);

    draw_scroll_text(dg_ptr(dgroup, 0x2296 /* "SELECT PUZZLE" */), 0xa8, 0x27, 0xc0);
    draw_scroll_text(dg_ptr(dgroup, 0x22a4 /* "PASSWORD" */), 0x20, 0x13c, 0x60);

    draw_sunken_box(0x1cc, 0x42, 0x20, 0x20);
    draw_sunken_box(0x1cc, 0x108, 0x20, 0x20);
    draw_sunken_box(0x1f0, 0x12c, 0x28, 0x28);

    puzzle_draw_up();
    puzzle_draw_down();
    puzzle_draw_ok(0);
    puzzle_draw_password(0x542e);
    puzzle_draw_list(DG53FC.word_542c, DG53FC.selected_level);

    present_back_page();
}

/*
 * 0x0f640
 *
 * **The password field**, and the third of these after the picker's two. Same
 * shape - copy, walk the pointer while the text is too wide, blink a caret by
 * counting - with its own numbers: a 0x122-wide field, the counter at 0x5428,
 * the asterisk at 0x2620, and the mode 0x800.
 *
 * Unlike the picker's two it takes the buffer as an **argument** rather than
 * naming it, because the one caller has it in a local at 0x542e and the field
 * is the only thing that reads it.
 *
 * It sets the text colour and **not** the background byte at 0x3891, where the
 * picker's fields set both. Whatever 0x3891 was left holding by the last thing
 * drawn is what this uses.
 */
void puzzle_draw_password(uint16_t text)
{
    uint8_t buf[40];                  /* [bp-0x28] */
    uint8_t *si  = buf;

    string_copy((volatile uint8_t *)buf, dg_ptr(dgroup, text));

    while ((int16_t)text_width_thunk(si) > 0x122)
        si++;

    if (DG4E67.state == 0x800) {
        DG53FC.word_5428++;
        if ((DG53FC.word_5428 & 8) != 0)
            string_concat(si, dg_ptr(dgroup, 0x2620 /* "*" */));
    }

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    fill_panel_area(0x90, 0x13c, 0x130, 0x10, 0);

    DG3890.unknown_00 = 0x0f;

    clear_flag_2d44_thunk();
    draw_string(si, 0x94, 0x140);
    restore_cursor_following();
}

/*
 * 0x0f6cc
 *
 * **The puzzle list.** 0x15 rows, each one built up in a local: `"PUZZLE "`,
 * the number, `": "`, and then the title out of the puzzle's own file.
 *
 * **A missing file ends the list**, and it ends it by setting the loop counter
 * to 0x34 - past its own limit of 0x15 - rather than by breaking. So the number
 * of puzzles is however many `l<n>.lev` files are actually present, and the
 * list finds out by asking rather than by being told.
 *
 * Three colours, and the middle one is the interesting one: white for the row
 * that is the current selection, **0x0a for a puzzle at or below the furthest
 * reached** at DGROUP 0x4eb7, and 0x0c for one beyond it. So the list shows
 * where the player has got to as well as where they are.
 */
void puzzle_draw_list(int16_t first, int16_t selected)
{
    uint8_t title[80];                    /* [bp-0xbe] */
    uint8_t name[100]; /* [bp-0x6e] */
    uint8_t num[10]; /* [bp-0x0a] */
    int16_t  i     = 0;
    int16_t  y     = 0x4c;
    int16_t  n     = first;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    fill_panel_area(0x30, 0x48, 0x190, 0xd8, 0);

    while (i < 0x15) {
        string_copy((volatile uint8_t *)name, dg_ptr(dgroup, 0x21e2 /* "PUZZLE " */));
        int_to_string(n, (volatile uint8_t *)num, 10);
        string_concat((volatile uint8_t *)name, (volatile uint8_t *)num);
        string_concat((volatile uint8_t *)name, dg_ptr(dgroup, 0x2622 /* ": " */));

        if (get_puzzle_title(n, (volatile uint8_t *)title) == 0) {
            i = 0x34;
        } else {
            string_concat((volatile uint8_t *)name, (volatile uint8_t *)title);

            if (n == selected)
                DG3890.unknown_00 = 0x0f;
            else if (n <= DG4E67.furthest_level)
                DG3890.unknown_00 = 0x0a;
            else
                DG3890.unknown_00 = 0x0c;

            clear_flag_2d44_thunk();
            draw_string((volatile uint8_t *)name, 0x34, y);
            restore_cursor_following();
        }

        i++;
        y = (int16_t)(y + 0x0a);
        n++;
    }
}

/*
 * 0x0f0a6
 *
 * **Take the round down**, and it is one call: `free_all_lists`. Nothing else
 * happens - no saving, no drawing, no state reset. Everything a round owns is
 * on those lists, and everything else it touched belongs to the game rather
 * than to the round.
 *
 * It is a routine rather than a call because `game_round` ends in one place and
 * `screen_state_0100` and the freeform handlers end a round in others.
 */
void round_teardown(void)
{
    free_all_lists();
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
    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        s->held = 0;
        DG4E67.state = 2;
    } else {
        if (s->held % 8 == 0 && ((int16_t)DG4E67.master_level) != 6) {
            DG4E67.master_level++;
            sub_12bed();
            set_master_level_ok(DGU16((uint16_t)(0x116 + 2 * DG4E67.master_level)));
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
    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        s->held = 0;
        DG4E67.state = 2;
    } else {
        if (s->held % 8 == 0 && ((int16_t)DG4E67.master_level) != 0) {
            DG4E67.master_level--;
            sub_12bed();
            set_master_level_ok(DGU16((uint16_t)(0x116 + 2 * DG4E67.master_level)));
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
        DG4E67.state = 1;
        s->done = 1;
    } else {
        DG4E67.state = 2;
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
        DG4E67.state = 0x1000;
        s->done = 1;
    } else {
        DG4E67.state = 2;
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
    if (DG4E67.round_kind != 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_level(1);
    present_back_page();

    if (ask_yes_no(0x1e26, 0x1e34)) {   /* "FREEFORM MODE" */
        round_teardown();
        load_animation(0x2824);         /* "ff.lev" */
        reset_machine();

        DG4E67.round_kind = 1;
        DG4E67.counter = 0;
        DG50AF.bonus_b = 0;
        DG50AF.bonus_a = 0;

        start_counters();
    }

    s->repaint_all = 1;
    DG4E67.state = 2;
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

    if (DG4E67.round_kind != 0) {
        if (ask_yes_no(0x1e62, 0x1e76)) {   /* "LEAVE FREEFORM MODE" */
            DG4E67.round_kind = 0;
            s->reload = 1;
        }
    }

    if (DG4E67.round_kind == 0) {
        if (sub_0f0b0() != 0 || s->reload != 0) {
            round_teardown();
            load_level(((uint16_t)DG4E67.round_number));
            reset_machine();
            s->reload = 0;
            start_counters();
        }
    }

    s->repaint_all = 1;
    DG4E67.state = 2;
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
    if (DG4E67.round_kind == 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_free_a(1);
    present_back_page();

    DG4E67.file_op_active = 1;
    if (dos_chdir(dg_off(dgroup, DG530B.picker_dir)) == 0)
        dos_setdisk((uint8_t)DG530B.picker_dir[0]);
    DG4E67.file_op_active = 0;

    if (pick_file(0, 0, 0x282b)) {      /* "*.TIM" */
        round_teardown();
        load_animation(dg_off(dgroup, DG52FE.name));
        reset_machine();
    }

    DG4E67.file_op_active = 1;
    dos_get_cur_dir(dg_off(dgroup, DG530B.picker_dir));
    if (dos_chdir(dg_off(dgroup, DG530B.game_dir)) == 0)
        dos_setdisk((uint8_t)DG530B.game_dir[0]);
    DG4E67.file_op_active = 0;

    s->repaint_all = 1;
    DG4E67.state = 2;
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
    if (DG4E67.round_kind == 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_free_b(1);
    present_back_page();

    DG4E67.file_op_active = 1;
    if (dos_chdir(dg_off(dgroup, DG530B.picker_dir)) == 0)
        dos_setdisk((uint8_t)DG530B.picker_dir[0]);
    DG4E67.file_op_active = 0;

    s->file_err = 1;
    while (s->file_err != 0) {
        DG4E67.state = 0x80;

        if (pick_file(0, 0, 0x2831)) {          /* "*.TIM" */
            s->file_err = save_machine(dg_off(dgroup, DG52FE.name));
            if (s->file_err != 0) {
                show_message_box(0x1fa0, 0x1ff6);   /* "FILE ERROR" */
                paint_game_screen(0);
            }
        } else {
            s->file_err = 0;
        }
    }

    dos_get_cur_dir(dg_off(dgroup, DG530B.picker_dir));
    DG4E67.file_op_active = 1;
    if (dos_chdir(dg_off(dgroup, DG530B.game_dir)) == 0)
        dos_setdisk((uint8_t)DG530B.game_dir[0]);
    DG4E67.file_op_active = 0;

    s->repaint_all = 1;
    DG4E67.state = 2;
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

    if (DG4E67.round_kind == 0) {
        /* "CAN'T CHANGE GRAVITY" */
        show_message_box(0x1ea4, 0x1eb9);
        s->repaint_all = 1;
        DG4E67.state = 2;
        return;
    }

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG4E67.state = 2;
        return;
    }

    v = long_divide(mul16x16((int16_t)(DG5768.pointer_x - 67), 0x200),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x200)
        v = 0x200;

    if ((int16_t)v != DG50AF.air) {
        DG50AF.air = (int16_t)v;
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

    if (DG4E67.round_kind == 0) {
        /* "CAN'T CHANGE AIR PRESSURE" */
        show_message_box(0x1f02, 0x1f1c);
        s->repaint_all = 1;
        DG4E67.state = 2;
        return;
    }

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG4E67.state = 2;
        return;
    }

    v = long_divide(mul16x16((int16_t)(DG5768.pointer_x - 67), 0x80),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x80)
        v = 0x80;

    if ((int16_t)v != DG50AF.gravity) {
        DG50AF.gravity = (int16_t)v;
        s->repaint_g = 2;
        recompute_kind_physics();
    }
}

/*
 * 0x114db
 *
 * **The enter-freeform region's handler**, and the first of five that are the
 * same eleven instructions with one constant changed: write a cursor number
 * into the region's own +0x0e, which `regions_handle_pointer` selects a moment
 * later.
 *
 * They are far pointers in the region table rather than a field because the
 * answer depends on the mode - freeform or not, DGROUP 0x4e67 - and a table
 * cannot hold a condition.
 *
 * **This one is the other way round from the four below**, and that is the
 * point: it has a cursor *outside* freeform and none inside it, because outside
 * is when the button does something. The other four have a cursor inside
 * freeform and none outside, for the same reason - Load, Save and the two
 * sliders only work there.
 *
 * The region it belongs to is the one whose +0x10 is 0x400, which is
 * `screen_state_0400`. It was called `restart` here at first, from the state
 * number rather than the table; the restart region is the one with 0x800, and
 * it has **no** handler at all.
 *
 * All five regions stay clickable either way: the mode a region switches to is
 * at +0x10 and none of these touches it.
 */
void region_cursor_freeform(uint16_t region)
{
    REGION(region).cursor = (DG4E67.round_kind != 0) ? 0 : 0x14;
}

/*
 * 0x114f8
 *
 * Load Machine's, and `region_cursor_freeform`'s twin the other way round: a
 * cursor in freeform, nothing outside it.
 */
void region_cursor_load(uint16_t region)
{
    REGION(region).cursor = (DG4E67.round_kind != 0) ? 0x17 : 0;
}

/*
 * 0x11515
 *
 * Save Machine's.
 */
void region_cursor_save(uint16_t region)
{
    REGION(region).cursor = (DG4E67.round_kind != 0) ? 0x16 : 0;
}

/*
 * 0x11532
 *
 * The gravity slider's.
 */
void region_cursor_gravity(uint16_t region)
{
    REGION(region).cursor = (DG4E67.round_kind != 0) ? 0x18 : 0;
}

/*
 * 0x1154f
 *
 * The air-pressure slider's.
 */
void region_cursor_air(uint16_t region)
{
    REGION(region).cursor = (DG4E67.round_kind != 0) ? 0x19 : 0;
}

/*
 * 0x1156c
 *
 * **Tab moves the mouse pointer**, not a focus ring. There is no keyboard
 * selection in this panel at all: Tab advances a cursor at DGROUP 0x27ee and
 * then *warps the pointer* to where that control is, so everything downstream -
 * the region test, the handlers this file is full of - carries on believing the
 * player moved the mouse there.
 *
 * The stop it lands on comes from a pair of tables in DGROUP, x at **0x27f0**
 * and y at **0x2802**. They are not the same length: x runs out after nine
 * entries, exactly where y's tenth begins, because stops 9 and 0x0a are the two
 * sliders and their x is *computed* from the value the slider is showing. Each
 * is the slider handler's own arithmetic run backwards - `* 0xa0 / 0x200 + 67`
 * against the handler's `(x - 67) * 0x200 / 0xa0` - so the pointer lands on the
 * knob where it already is and the next Tab does not move it.
 *
 * Which stops exist depends on the mode: in freeform (DGROUP 0x4e67) the cycle
 * is 0..0x0a with 5 skipped, and outside it the cycle stops at 7 and wraps.
 * Both wrap checks run, because the increment can arrive at 0x0b from either.
 */
void sub_1156c(void)
{
    int16_t  x;
    uint16_t stop;

    DG27EE.word_27ee++;

    if (DG4E67.round_kind != 0) {
        if (DG27EE.word_27ee == 5)
            DG27EE.word_27ee = 6;
    } else if (DG27EE.word_27ee == 7) {
        DG27EE.word_27ee = 0;
    }

    if (DG27EE.word_27ee == 0x0b)
        DG27EE.word_27ee = 0;

    stop = DG27EE.word_27ee;

    if (stop == 9)
        x = (int16_t)long_divide(mul16x16(DG50AF.air, 0xa0), 0x200)
            + 0x43;
    else if (stop == 0x0a)
        x = (int16_t)long_divide(mul16x16(DG50AF.gravity, 0xa0), 0x80)
            + 0x43;
    else
        x = DG16((uint16_t)(0x27f0 + 2 * stop));

    move_pointer_to(x, DG16((uint16_t)(0x2802 + 2 * stop)));
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

    saved = DG4E67.state;
    DG4E67.state = 0x8000;

    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(dg_ptr(dgroup, title), 0xb8, 0x74, 0xd0);
    draw_panel(0xb8, 0x90, 0xd0, 0x5a);
    draw_wrapped_text(body, 0xbc, 0x94, 0xc8, 0x30);

    draw_button(button1, 0xc8, 0xd4, 0);
    DGU16((uint16_t)(DG4E67.region_kept_b_ptr + 0x0a)) =
        (uint16_t)(text_width_thunk(dg_ptr(dgroup, button1)) + 0xd8);

    if (button2 != 0) {
        second_x = (int16_t)(0x168
                             - ((text_width_thunk(dg_ptr(dgroup, button2)) + 7) & 0xfff8));
        draw_button(button2, (uint16_t)second_x, 0xd4, 0);
        DGU16((uint16_t)(DG4E67.region_kept_a_ptr + 6)) = (uint16_t)second_x;
    }

    present_back_page();
    restore_cursor();

    while (DG4E67.state == 0x8000) {
        update_button_state();

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);

        if (((uint8_t)DG52ED.last_key) == 0x0f) {
            message_box_tab(button2);
        } else {
            if (DG8(button1) == 'Y') {
                if (((uint8_t)DG52ED.last_key) == 0x15)
                    DG4E67.state = 0x4000;
                if (((uint8_t)DG52ED.last_key) == 0x31)
                    DG4E67.state = 0x2000;
            }
            if (DG8(button1) == 'R') {
                if (((uint8_t)DG52ED.last_key) == 0x13)
                    DG4E67.state = 0x4000;
                if (((uint8_t)DG52ED.last_key) == 0x1e)
                    DG4E67.state = 0x2000;
            }
            if (DG8(button1) == 'C') {
                if (((uint8_t)DG52ED.last_key) == 0x2e)
                    DG4E67.state = 0x4000;
                if (((uint8_t)DG52ED.last_key) == 0x1c)
                    DG4E67.state = 0x4000;
            }
        }

        regions_handle_pointer(DG4E67.regions_b_ptr);

        if (button2 == 0 && DG4E67.state == 0x2000)
            DG4E67.state = 0x8000;

        present_frame(1);
    }

    update_button_state();

    if (DG4E67.state == 0x4000) {
        draw_button(button1, 0xc8, 0xd4, 1);
        present_back_page();
        DG4E67.state = saved;
        return 1;
    }

    if (button2 != 0) {
        draw_button(button2, (uint16_t)second_x, 0xd4, 1);
        present_back_page();
    }
    DG4E67.state = saved;
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
    DG259C.word_259c++;

    if (button2 != 0) {
        if (DG259C.word_259c == 2)
            DG259C.word_259c = 0;
    } else {
        DG259C.word_259c = 0;
    }

    move_pointer_to((int16_t)DGU16((uint16_t)(0x259e + 2 * DG259C.word_259c)),
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
 * OURS: what both resize arms do once they have decided which way to go.
 *
 * The original writes these four calls out twice in each arm - once for the
 * width and once for the height - so four copies in all, identical but for the
 * field they follow. Factored here because the *decision* above it is the part
 * that differs, and that is left written out.
 */
static void carried_part_resized(struct part *part, uint16_t kind)
{
    call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, dg_off(dgroup, part), "settle");
    place_object_for_draw(part);
    mark_needs_refile(part, 2);
    mark_joined_shapes(part, 3);
}

/*
 * 0x10466
 *
 * **Grow the part in your hand** - the `=` and `+` arm of
 * `part_key_shortcut`, scancodes 13 and 78.
 *
 * Which axis grows is decided first, and it is not a choice the player makes:
 * the **shorter side grows**, so +0x50 unless +0x52 is already bigger. Kind 2
 * is the exception and always takes the width, whatever its height is.
 *
 * Then the chosen side moves by 0x10 if the kind's maximum leaves room -
 * `cs:0x0eb2` for the width and `cs:0x0eb4` for the height, both compared
 * signed - and +0x40 or +0x42 is brought along to match. Nothing happens at
 * all when the part is already at its limit; there is no clamp, just no step.
 *
 * The original holds the kind in SI, loaded by `part_key_shortcut` and, as the
 * note there says, never used by that routine itself - only by these two arms.
 * It is the part's own +4, so it is recomputed here rather than passed.
 */
void carried_part_grow(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    uint16_t kind = (uint16_t)((int16_t)PART(part).kind * 0x3a);

    if ((int16_t)PART(part).word_52
            <= (int16_t)PART(part).word_50
        || PART(part).kind == KIND_RAMP) {
        if ((int16_t)(PARTKIND_AT(0x0ea6 + kind).max_w)
                > (int16_t)PART(part).word_50) {
            PART(part).word_50 =
                (uint16_t)(PART(part).word_50 + 0x10);
            PART(part).word_40 = PART(part).word_50;
            carried_part_resized(PARTP(part), kind);
        }
    } else {
        if ((int16_t)(PARTKIND_AT(0x0ea6 + kind).max_h)
                > (int16_t)PART(part).word_52) {
            PART(part).word_52 =
                (uint16_t)(PART(part).word_52 + 0x10);
            PART(part).word_42 = PART(part).word_52;
            carried_part_resized(PARTP(part), kind);
        }
    }
}

/*
 * 0x10551
 *
 * **Shrink the part in your hand** - the `-` arm, scancodes 12 and 74, and the
 * mirror of `carried_part_grow` instruction for instruction: the same choice of
 * axis, the minimum at `cs:0x0eb6` and `cs:0x0eb8` instead of the maximum, the
 * comparison the other way round, and 0x10 subtracted rather than added.
 */
void carried_part_shrink(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    uint16_t kind = (uint16_t)((int16_t)PART(part).kind * 0x3a);

    if ((int16_t)PART(part).word_52
            <= (int16_t)PART(part).word_50
        || PART(part).kind == KIND_RAMP) {
        if ((int16_t)(PARTKIND_AT(0x0ea6 + kind).min_w)
                < (int16_t)PART(part).word_50) {
            PART(part).word_50 =
                (uint16_t)(PART(part).word_50 - 0x10);
            PART(part).word_40 = PART(part).word_50;
            carried_part_resized(PARTP(part), kind);
        }
    } else {
        if ((int16_t)(PARTKIND_AT(0x0ea6 + kind).min_h)
                < (int16_t)PART(part).word_52) {
            PART(part).word_52 =
                (uint16_t)(PART(part).word_52 - 0x10);
            PART(part).word_42 = PART(part).word_52;
            carried_part_resized(PARTP(part), kind);
        }
    }
}

/*
 * 0x0fe47
 *
 * Move whatever is in your hand, by kind. Tool 9's arm of the level loop.
 *
 * **+0x20 and +0x1e are set to -1 first**, both of them, before anything looks
 * at the kind. A carried part has no position until the mover gives it one,
 * and -1 is what the drawing code reads as "nowhere yet" - so a frame that
 * ends up not placing it leaves it off the board rather than at its old spot.
 */
void move_carried(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;

    PART(part).pos_y = -1;
    PART(part).pos_x = -1;

    if (PART(part).kind == KIND_BELT)
        move_carried_rope();
    else if (PART(part).kind == KIND_ROPE)
        move_carried_belt();
    else
        move_carried_part();
}

/*
 * 0x101dc
 *
 * **Move an ordinary carried part** with the pointer - everything that is not
 * a rope or a belt - and put it down when the button goes down.
 *
 * The level's own opinion is asked first, through `part_key_shortcut`,
 * which does nothing at all on all but six levels.
 *
 * Then the position, and bit 8 of +0xa decides which of two quite different
 * ways: **free** placement follows the pointer exactly and is clamped into the
 * play area by 0xc at the near edges and 0x235 and 0x165 at the far ones;
 * **snapped** placement masks the pointer to a multiple of 16 and, if the part
 * would end up entirely off the near edge, nudges it back by one whole cell
 * rather than clamping. Both take the grab offset at 0x4e97 and 0x4e95 off
 * first, so the part stays held where it was picked up.
 *
 * `di` is whether the part's rope is *not* close enough to stay joined -
 * `neg/sbb/inc` around `rope_ends_close`, which is Borland's `== 0` - and it
 * is only consulted when the part is actually put down.
 *
 * Then +0xa again: bit 1 re-homes the part onto whatever it is near, bit 2
 * goes to sub_051cb instead, and neither is tried if the other matched.
 *
 * The ending is three-way. Overlapping something sets the cursor colour at
 * 0x52c7 to 0xe and nothing else happens - you cannot drop a part inside
 * another. The button down commits: marks, then **a rope that has come too far
 * apart is untied and thrown away**, then +0x8c and +0x8e remember where the
 * part landed, it is re-filed, and the hand is emptied. Neither of those and
 * the colour is 0xc, meaning it would drop cleanly.
 *
 * `part_moved` runs on every frame the button is *not* down, which is to say
 * while it is still being dragged rather than when it lands.
 */
void move_carried_part(void)
{
    uint16_t part;
    uint16_t si;
    int16_t di;

    part_key_shortcut();

    part = DG50D3.dragged_part_ptr;

    if (PART(part).flags_0a & 8) {
        PART(part).pos_x =
            (uint16_t)(((uint16_t)DG5768.pointer_x) - DG4E67.word_4e97 + ((uint16_t)DG4E67.origin_x));

        if ((int16_t)(((uint16_t)PART(part).pos_x)
                      + ((uint16_t)PART(part).width))
            <= (int16_t)(((uint16_t)DG4E67.origin_x) + 0x0c))
            PART(part).pos_x =
                (uint16_t)(((uint16_t)DG4E67.origin_x) - ((uint16_t)PART(part).width)
                           + 12);

        if ((int16_t)((uint16_t)PART(part).pos_x)
            >= (int16_t)(((uint16_t)DG4E67.origin_x) + 0x235))
            PART(part).pos_x =
                (uint16_t)(((uint16_t)DG4E67.origin_x) + 565);

        PART(part).pos_y =
            (uint16_t)(((uint16_t)DG5768.pointer_y) - DG4E67.word_4e95 + ((uint16_t)DG4E67.origin_y));

        if ((int16_t)(((uint16_t)PART(part).pos_y)
                      + ((uint16_t)PART(part).height))
            <= (int16_t)(((uint16_t)DG4E67.origin_y) + 0x0c))
            PART(part).pos_y =
                (uint16_t)(((uint16_t)DG4E67.origin_y) - ((uint16_t)PART(part).height)
                           + 12);

        if ((int16_t)((uint16_t)PART(part).pos_y)
            >= (int16_t)(((uint16_t)DG4E67.origin_y) + 0x165))
            PART(part).pos_y =
                (uint16_t)(((uint16_t)DG4E67.origin_y) + 357);
    } else {
        PART(part).pos_x =
            (uint16_t)(((((uint16_t)DG5768.pointer_x) - DG4E67.word_4e97) & 0xfff0)
                       + ((uint16_t)DG4E67.origin_x));
        if ((int16_t)(((uint16_t)PART(part).pos_x)
                      + ((uint16_t)PART(part).width))
            <= (int16_t)((uint16_t)DG4E67.origin_x))
            PART(part).pos_x =
                (uint16_t)(((uint16_t)PART(part).pos_x) + 16);

        PART(part).pos_y =
            (uint16_t)(((((uint16_t)DG5768.pointer_y) - DG4E67.word_4e95) & 0xfff0)
                       + ((uint16_t)DG4E67.origin_y));
        if ((int16_t)(((uint16_t)PART(part).pos_y)
                      + ((uint16_t)PART(part).height))
            <= (int16_t)((uint16_t)DG4E67.origin_y))
            PART(part).pos_y =
                (uint16_t)(((uint16_t)PART(part).pos_y) + 16);
    }

    place_object_for_draw(PARTP(part));
    retension_pulleys(PARTP(part));

    si = PART(part).word_54;
    di = (si != 0) ? (int16_t)(rope_ends_close(si) == 0) : 0;

    if (PART(part).flags_0a & 1)
        rehome_carried_part();
    else if (PART(part).flags_0a & 2)
        sub_051cb(PARTP(part));

    if (object_overlaps_any(PARTP(part)) != 0) {
        DG52BD.drop_cursor = 0x0e;
    } else if (DG5768.button_left == 2) {
        mark_joined_shapes(PARTP(part), 3);

        if (di != 0) {
            untie_rope(PARTP(ROPE(si).owner_ptr));
            discard_part(PARTP(ROPE(si).owner_ptr));
            DG4E67.redraw_e = 2;
        }

        mark_needs_refile(PARTP(part), 2);
        PART(part).word_8c = ((uint16_t)PART(part).pos_x);
        PART(part).word_8e = ((uint16_t)PART(part).pos_y);
        refile_part_list(PARTP(part));
        DG4E67.word_4e69 = 0;
        DG50D3.dragged_part_ptr = 0;
    } else {
        DG52BD.drop_cursor = 0x0c;
    }

    if (DG5768.button_left != 2)
        part_moved(PARTP(part));
}

/*
 * 0x10410
 *
 * **The keyboard shortcuts for the part in your hand.** DGROUP 0x52f1 is the
 * last key, and six scancodes have a meaning here; every other key falls
 * straight out, which is what this routine does almost every frame.
 *
 * The table of six at CS:0x2650 is searched with a `loop` and a hit jumps
 * through the parallel table twelve bytes further on. Read as scancodes it is
 * obvious what they are:
 *
 *     45  X          flip the first end, if the part has one
 *     21  Y          flip the second end, if the part has one
 *     13  =   78  +  grow the part in your hand
 *     12  -   74  -  shrink it
 *
 * X and Y flipping the two axes is what identifies the table; as level
 * numbers - which is how this was first written up, because 0x52f1 was
 * mistaken for the level - 12, 13, 21, 45, 74 and 78 look like nothing at all.
 *
 * The two flip arms are here in full because they are five instructions each;
 * the resize pair are ~235 bytes apiece and have routines of their own.
 *
 * `si` is loaded with the part's kind at entry and never used. That is the
 * original's, not an omission.
 */
void part_key_shortcut(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    static const uint16_t KEYS[6] = { 12, 13, 21, 45, 74, 78 };
    uint16_t key = ((uint8_t)DG52ED.last_key);
    int32_t i;

    for (i = 0; i < 6; i++)
        if (KEYS[i] == key)
            break;

    if (i == 6)
        return;

    switch (KEYS[i]) {
    case 45:
        if (PART(part).flags_06 & 0x400)
            flip_carried_end_1();
        return;
    case 21:
        if (PART(part).flags_06 & 0x200)
            flip_carried_end_2();
        return;
    case 13:
    case 78:
        carried_part_grow();
        return;
    case 12:
    case 74:
        carried_part_shrink();
        return;
    default:
        return;
    }
}

/*
 * 0x10658
 *
 * **Pick a placed part up** and start carrying it - tool 7's arm, taken when
 * the button goes down on a part's body.
 *
 * The grab offset is saved first: 0x4e97 and 0x4e95 are the pointer less the
 * part's own origin, so a part picked up by its corner stays held by its
 * corner however far the pointer then moves.
 *
 * `di` is the part's +0x54 and `si` **its +4, read only if +0x54 is not zero**.
 * `si` is used again at the end, and only on the branch where the part is a
 * rope - which is exactly when +0x54 is set - so the original's conditional
 * load is safe. It is initialised to 0 here because C says so; the original
 * would be carrying whatever SI held.
 *
 * The part is unmarked, then detached according to kind: a rope untied, a belt
 * detached with `how` 0 after stashing its far end's +0x5a at 0x5456, anything
 * else through sub_05704. A rope then has its link put back the other way
 * round - `di->+4 = si`, `si->+0x54 = di` - with bit 2 set in the far part's
 * +8 and +0x94 refreshed to match, so the rope is now held by the end you did
 * not grab.
 *
 * Tool 9 last, which is what makes everything else treat this as carried.
 */
void pick_up_part(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    uint16_t di, si = 0, rec, idx;

    DG4E67.word_4e97 = (uint16_t)(((uint16_t)DG5768.pointer_x)
                               - ((uint16_t)PART(part).pos_x)
                               + ((uint16_t)DG4E67.origin_x));
    DG4E67.word_4e95 = (uint16_t)(((uint16_t)DG5768.pointer_y)
                               - ((uint16_t)PART(part).pos_y)
                               + ((uint16_t)DG4E67.origin_y));

    di = PART(part).word_54;
    if (di != 0)
        si = PART(di).kind;

    mark_joined_shapes(PARTP(part), 3);
    mark_part_shapes(PARTP(part), 3);

    if (PART(part).kind == KIND_BELT) {
        untie_rope(PARTP(part));
    } else if (PART(part).kind == KIND_ROPE) {
        rec = PART(part).word_66;
        idx = ((int8_t)BELT(rec).slot_b);
        DG5456.belt_far_end = DGU16((uint16_t)(BELT(rec).end_b_ptr
                                         + idx * 2 + 0x5a));
        detach_belt(PARTP(part), 0);
    } else {
        sub_05704(PARTP(part));
    }

    if (PART(part).kind == KIND_BELT) {
        PART(di).kind = si;
        PART(si).flags_08 |= 2;
        PART(si).word_94 = PART(si).flags_08;
        PART(si).word_54 = di;
    }

    DG4E67.word_4e69 = 9;
}

/*
 * 0x10733
 *
 * **Throw away the part in hand**, whatever kind it is, and leave the player
 * holding nothing.
 *
 * Its shapes are unmarked first - `mark_joined_shapes` and `mark_part_shapes`
 * both with mode 3 - so nothing that was drawn for it is left claiming space.
 * Then three ways to go, on the kind at +4:
 *
 *   a rope, kind 8      untie it from both ends, then discard
 *   a belt, kind 0x0a   detach it with `how` 1, then discard
 *   anything else       sub_05704 and sub_05482
 *
 * The two ends of the first two are why they need untying before discarding: a
 * rope or a belt is joined to parts that outlive it, and freeing the record
 * without breaking the joins leaves those parts pointing at it.
 *
 * It closes by setting 0x4e93 to 2 and the tool at 0x4e69 to 0 - the hand is
 * empty, so no tool is selected. Both are unconditional and outside the
 * branch, and are transcribed there.
 */
void discard_carried_part(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;

    mark_joined_shapes(PARTP(part), 3);
    mark_part_shapes(PARTP(part), 3);

    if (PART(part).kind == KIND_BELT) {
        untie_rope(PARTP(part));
        discard_part(PARTP(part));
    } else if (PART(part).kind == KIND_ROPE) {
        detach_belt(PARTP(part), 1);
        discard_part(PARTP(part));
    } else {
        sub_05704(PARTP(part));
        sub_05482();
    }

    DG4E67.redraw_e = 2;
    DG4E67.word_4e69 = 0;
}

/*
 * 0x107b6
 *
 * Flip the carried part's **first** end, for real - the arm the level loop
 * takes for tool 1 when the button has just gone down.
 *
 * The same flip hook `part_flip_options` uses to *test* an end, called once
 * with 1 and not undone, so this is the move rather than the trial. +0x94 is
 * refreshed from +8 afterwards for the same reason it is there: the hook
 * changes +8 and the two must not drift apart.
 */
void flip_carried_end_1(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    uint16_t kind = (uint16_t)((int16_t)PART(part).kind * 0x3a);

    call_part_flip(PARTKIND_AT(0x0ea6 + kind).flip, part, 1);
    PART(part).word_94 = PART(part).flags_08;
}

/*
 * 0x107e6
 *
 * The **second** end, and `flip_carried_end_1` with a 2 in it - tool 2's arm.
 * Kept as two routines because the original has two; they differ in one
 * immediate and nothing else.
 */
void flip_carried_end_2(void)
{
    uint16_t part = DG50D3.dragged_part_ptr;
    uint16_t kind = (uint16_t)((int16_t)PART(part).kind * 0x3a);

    call_part_flip(PARTKIND_AT(0x0ea6 + kind).flip, part, 2);
    PART(part).word_94 = PART(part).flags_08;
}

/*
 * 0x10816
 *
 * **Run one frame of a drag.** The level loop's arm for tools 3 to 6, and the
 * only place the four drag routines are called from.
 *
 * The top bit of 0x4e69 is what says a drag is in progress. Without it, this
 * does one thing: if the button has just gone down, set the bit and return -
 * so the frame that starts a drag does no dragging.
 *
 * With it, the low bits pick one of four through a jump table at CS:0x28f4,
 * indexed by `0x4e69 - 0x8003`, and anything outside 0..3 falls through with
 * nothing moved rather than being rejected:
 *
 *     0x8003  drag_carried_part_first     0x8005  drag_carried_part_pair
 *     0x8004  settle_carried_part_first   0x8006  settle_carried_part
 *
 * If the part moved, +0x42 and +0x40 take copies of +0x52 and +0x50 - the
 * position the *next* frame will treat as where it came from - and the part is
 * re-hooked, re-placed and marked three ways.
 *
 * The button being down **ends** the drag, clearing both the tool and the
 * carried part. That is not a mistake: 0x5774 is 2 only on the press edge, so
 * the drag runs while the button is up and the next press drops it.
 */
void run_drag_frame(void)
{
    int16_t si = 0;
    uint16_t part, kind;

    if ((DG4E67.word_4e69 & 0x8000) == 0) {
        if (DG5768.button_left == 2)
            DG4E67.word_4e69 |= 0x8000;
        return;
    }

    switch ((uint16_t)(DG4E67.word_4e69 - 0x8003)) {
    case 0: si = drag_carried_part_first();   break;
    case 1: si = settle_carried_part_first(); break;
    case 2: si = drag_carried_part_pair();    break;
    case 3: si = settle_carried_part();       break;
    default: break;
    }

    if (si != 0) {
        part = DG50D3.dragged_part_ptr;
        kind = (uint16_t)((int16_t)PART(part).kind * 0x3a);

        PART(part).word_42 = PART(part).word_52;
        PART(part).word_40 = PART(part).word_50;

        call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, part, "settle");
        place_object_for_draw(PARTP(part));
        mark_joined_shapes(PARTP(part), 3);
        mark_part_shapes(PARTP(part), 3);
        mark_needs_refile(PARTP(part), 2);
    }

    if (DG5768.button_left == 2) {
        DG4E67.word_4e69 = 0;
        DG50D3.dragged_part_ptr = 0;
    }
}

/*
 * 0x108ec
 *
 * Drag the carried part by its **first** pair - `drag_carried_part_pair`'s
 * sibling, on +0x1e and +0x50 rather than +0x20 and +0x52, driven by the other
 * pointer axis and clamped against the kind's other pair of bounds, +0x0c and
 * +0x10. It remembers into +0x8c where the other remembers into +0x8e.
 *
 * The four differ only in which words they touch; each is written out rather
 * than folded into one routine taking offsets, because that is how the
 * original has them and a table of offsets would be a different program that
 * happens to agree.
 */
int16_t drag_carried_part_first(void)
{
    uint16_t moved;                    /* [bp-8] */
    uint16_t hi;    /* [bp-6] */
    uint16_t lo;    /* [bp-4] */
    uint16_t was;    /* [bp-2] */
    uint16_t part  = DG50D3.dragged_part_ptr;
    uint16_t kind  = (uint16_t)((int16_t)PART(part).kind * 0x3a);
    int16_t  si, di;

    moved = 0;
    was = ((uint16_t)PART(part).pos_x);

    si = (int16_t)((((uint16_t)DG5768.pointer_x) & 0xfff0) + ((uint16_t)DG4E67.origin_x));

    lo = ((uint16_t)PARTKIND_AT(0x0ea6 + kind).min_w);
    hi = ((uint16_t)PARTKIND_AT(0x0ea6 + kind).max_w);

    di = (int16_t)(was - si + PART(part).word_50);

    if (di > (int16_t)hi) {
        si = (int16_t)(si + (di - (int16_t)hi));
        di = (int16_t)hi;
    } else if (di < (int16_t)lo) {
        si = (int16_t)(si - ((int16_t)lo - di));
        di = (int16_t)lo;
    }

    if (was != (uint16_t)si) {
        PART(part).pos_x = (uint16_t)si;
        PART(part).word_50 = (uint16_t)di;

        for (;;) {
            call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, part, "settle");
            place_object_for_draw(PARTP(part));
            call_part_setup(PARTKIND_AT(0x0ea6 + kind).setup, part);
            if (object_overlaps_any(PARTP(part)) == 0)
                break;
            PART(part).pos_x =
                (uint16_t)(((uint16_t)PART(part).pos_x) + 16);
            PART(part).word_50 =
                (uint16_t)(PART(part).word_50 - 0x10);
        }

        if (((uint16_t)PART(part).pos_x) != was) {
            PART(part).word_8c = ((uint16_t)PART(part).pos_x);
            moved = 1;
        }
    }

    {
        int16_t answer = (int16_t)moved;

        return answer;
    }
}

/*
 * 0x10a00
 *
 * Settle the carried part on its **first** axis - `settle_carried_part`'s
 * sibling, on +0x50 and +0x1e rather than +0x52 and +0x20, taking its target
 * from 0x5784 and 0x4ea3 and clamping against the kind's +0x0c and +0x10.
 *
 * The fourth and last of the drag family, and like the other three it lifts by
 * a whole row at a time until `object_overlaps_any` is satisfied, with the
 * first placement before the first test - a do-while, as written.
 */
int16_t settle_carried_part_first(void)
{
    int16_t moved;                    /* [bp-6] */
    int16_t hi;    /* [bp-4] */
    int16_t lo;    /* [bp-2] */
    uint16_t part  = DG50D3.dragged_part_ptr;
    uint16_t kind  = (uint16_t)((int16_t)PART(part).kind * 0x3a);
    uint16_t was   = PART(part).word_50;
    int16_t  si;

    moved = (int16_t)0;

    si = (int16_t)((((uint16_t)DG5768.pointer_x) & 0xfff0) + ((uint16_t)DG4E67.origin_x) + 0x10
                   - ((uint16_t)PART(part).pos_x));

    lo = (int16_t)((uint16_t)PARTKIND_AT(0x0ea6 + kind).min_w);
    hi = (int16_t)((uint16_t)PARTKIND_AT(0x0ea6 + kind).max_w);

    if (si > (int16_t)(uint16_t)hi)
        si = (int16_t)(uint16_t)hi;
    else if (si < (int16_t)(uint16_t)lo)
        si = (int16_t)(uint16_t)lo;

    if (was != (uint16_t)si) {
        PART(part).word_50 = (uint16_t)si;

        for (;;) {
            call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, part, "settle");
            place_object_for_draw(PARTP(part));
            call_part_setup(PARTKIND_AT(0x0ea6 + kind).setup, part);
            if (object_overlaps_any(PARTP(part)) == 0)
                break;
            PART(part).word_50 =
                (uint16_t)(PART(part).word_50 - 0x10);
        }

        if (PART(part).word_50 != was)
            moved = (int16_t)1;
    }

    {
        int16_t answer = (int16_t)(uint16_t)moved;
        return answer;
    }
}

/*
 * 0x10ada
 *
 * Drag the carried part **along its other axis**, and say whether it moved -
 * `settle_carried_part`'s twin, and not a mirror of it.
 *
 * Where that one moves +0x52 alone, this moves +0x20 and +0x52 **together and
 * in opposite directions**: the new +0x20 comes from the pointer, +0x52 is
 * derived from it as `was + pointer_delta`, and the settling loop adds 0x10 to
 * one while taking 0x10 off the other. The pair is a diagonal, which is what a
 * part with two ends slides along.
 *
 * The clamp is on +0x52 against the same kind bounds at +0x12 and +0x0e, and
 * the *overshoot is pushed back into +0x20* rather than discarded - `si +=
 * di - hi` - so the two stay consistent when the end is pinned.
 *
 * On success +0x8e takes a copy of the new +0x20, which the plain vertical
 * drag does not do.
 */
int16_t drag_carried_part_pair(void)
{
    int16_t moved;                    /* [bp-8] */
    int16_t hi;    /* [bp-6] */
    int16_t lo;    /* [bp-4] */
    int16_t was;    /* [bp-2] */
    uint16_t part  = DG50D3.dragged_part_ptr;
    uint16_t kind  = (uint16_t)((int16_t)PART(part).kind * 0x3a);
    int16_t  si, di;

    moved = (int16_t)0;
    was = (int16_t)((uint16_t)PART(part).pos_y);

    si = (int16_t)((((uint16_t)DG5768.pointer_y) & 0xfff0) + ((uint16_t)DG4E67.origin_y));

    lo = (int16_t)((uint16_t)PARTKIND_AT(0x0ea6 + kind).min_h);
    hi = (int16_t)((uint16_t)PARTKIND_AT(0x0ea6 + kind).max_h);

    di = (int16_t)((uint16_t)was - si + PART(part).word_52);

    if (di > (int16_t)(uint16_t)hi) {
        si = (int16_t)(si + (di - (int16_t)(uint16_t)hi));
        di = (int16_t)(uint16_t)hi;
    } else if (di < (int16_t)(uint16_t)lo) {
        si = (int16_t)(si - ((int16_t)(uint16_t)lo - di));
        di = (int16_t)(uint16_t)lo;
    }

    if ((uint16_t)was != (uint16_t)si) {
        PART(part).pos_y = (uint16_t)si;
        PART(part).word_52 = (uint16_t)di;

        for (;;) {
            call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, part, "settle");
            place_object_for_draw(PARTP(part));
            call_part_setup(PARTKIND_AT(0x0ea6 + kind).setup, part);
            if (object_overlaps_any(PARTP(part)) == 0)
                break;
            PART(part).pos_y =
                (uint16_t)(((uint16_t)PART(part).pos_y) + 16);
            PART(part).word_52 =
                (uint16_t)(PART(part).word_52 - 0x10);
        }

        if (((uint16_t)PART(part).pos_y) != (uint16_t)was) {
            PART(part).word_8e = ((uint16_t)PART(part).pos_y);
            moved = (int16_t)1;
        }
    }

    {
        int16_t answer = (int16_t)(uint16_t)moved;
        return answer;
    }
}

/*
 * 0x10bee
 *
 * Drop the carried part onto something solid, and say whether it moved.
 *
 * Its y at +0x52 is first put where the pointer is - the pointer's row at
 * 0x5782 taken down to a multiple of 16, plus the play area's top at 0x4ea3
 * and one more row, less the part's own height at +0x20 - and then clamped
 * into the band its kind allows, +0x12 low and +0x0e high in the kind record.
 * The two are read through the usual `kind * 0x3a` stride.
 *
 * If that lands where it already was, nothing happens and the answer is 0.
 * Otherwise it settles: place it, ask `object_overlaps_any`, and while the
 * answer is yes lift it a whole row and ask again. The loop has no bound of
 * its own - it is the clamp above and the ceiling of the play area that end
 * it - and it is transcribed as the do-while the original writes, because the
 * first placement happens before the first test.
 *
 * Two of the three calls in the loop are per-kind hooks reached through far
 * pointers in the kind record, +0x32 and +0x2a, which C cannot call; they go
 * through `call_part_hook` and `call_part_setup` like every other one. For
 * every kind this game's early levels use, +0x32 is the do-nothing hook.
 *
 * The answer is whether the part ended up somewhere other than where it
 * started, which is not the same as whether the loop ran: a part lifted and
 * put back reports 0.
 */
int16_t settle_carried_part(void)
{
    uint16_t moved;                    /* [bp-6] */
    uint16_t hi;    /* [bp-4] */
    uint16_t lo;    /* [bp-2] */
    uint16_t part  = DG50D3.dragged_part_ptr;
    uint16_t was   = PART(part).word_52;
    uint16_t kind  = (uint16_t)((int16_t)PART(part).kind * 0x3a);
    int16_t  y;

    moved = 0;

    y = (int16_t)((((uint16_t)DG5768.pointer_y) & 0xfff0) + ((uint16_t)DG4E67.origin_x) + 0x10
                  - ((uint16_t)PART(part).pos_y));

    lo = ((uint16_t)PARTKIND_AT(0x0ea6 + kind).min_h);
    hi = ((uint16_t)PARTKIND_AT(0x0ea6 + kind).max_h);

    if (y > (int16_t)hi)
        y = (int16_t)hi;
    else if (y < (int16_t)lo)
        y = (int16_t)lo;

    if ((uint16_t)y != was) {
        PART(part).word_52 = (uint16_t)y;

        for (;;) {
            call_part_hook(PARTKIND_AT(0x0ea6 + kind).settle, part, "settle");
            place_object_for_draw(PARTP(part));
            call_part_setup(PARTKIND_AT(0x0ea6 + kind).setup, part);
            if (object_overlaps_any(PARTP(part)) == 0)
                break;
            PART(part).word_52 =
                (uint16_t)(PART(part).word_52 - 0x10);
        }

        if (PART(part).word_52 != was)
            moved = 1;
    }

    {
        int16_t answer = (int16_t)moved;

        return answer;
    }
}

/* The parts bin's initial repeat delay, in loop iterations. Ours - see below. */
#define BIN_REPEAT_DELAY 9

/*
 * OURS: not a transcription, but a **deliberate deviation** chosen by the
 * project owner on 2026-09-06 - the only one in this file.
 *
 * The original has no initial repeat delay. `bin_scroll_back` and
 * `bin_scroll_forward` fire on call 0, 3, 6 ... of `game_screen_loop` with the
 * counter reset only on release, so a press begins repeating at once - read
 * instruction by instruction against 0x10cc8 and 0x10d37, which match.
 *
 * That is fine on the machine it was written for and not on this one. The
 * frame wait at `game_screen_loop` is a **minimum** - eight ticks of a 236.7 Hz
 * timer - and a 386 spent longer than that on the frame itself, so its loop ran
 * slower than the 29.6 iterations a second the port achieves. At the port's
 * rate an ordinary click of about 150 ms spans four iterations, which is enough
 * for the counter to come round to 3 and scroll a second page. Measured: one
 * click gave one page about 20% of the time.
 *
 * So the press still fires immediately, then nothing until the delay, then the
 * original's one-in-three.
 *
 * **The delay is ours, and it is twelve loop iterations - about 400 ms.**
 *
 * An earlier version of this took the 12 from DGROUP 0x2d40, the original's
 * own constant, and said so as if that gave it provenance. It does not.
 * `button_state` reloads its per-button countdown from that word and decrements
 * it once per call, and it is called from `timer_callback` - so its unit is a
 * **timer tick at 236.7 Hz**, where 12 is about 51 ms. Using the same number as
 * a count of *loop iterations* at 29.6 Hz stretches it eightfold. The two
 * quantities are not the same quantity, and borrowing the digits was dressing a
 * chosen number as a measured one.
 *
 * In its own units it would not work either: 51 ms expires part-way through an
 * ordinary 150 ms click, which is 4.4 iterations here, so the counter would
 * still come round and scroll again.
 *
 * Twelve iterations is therefore chosen, on the only grounds that hold - it
 * sits past a click and short of a deliberate hold - and is written here as a
 * constant of ours rather than read from a word that means something else.
 *
 * The test is `n % 3` and not `(n - BIN_REPEAT_DELAY) % 3`. The subtraction
 * was there to make the first repeat land exactly at the end of the delay
 * whatever the delay was, and with a delay that is a multiple of three - which
 * this one is, and which the constant above asks it to stay - the two are the
 * same expression. Arithmetic that can never change an answer is worse than
 * none: it reads as though it matters.
 */
static int32_t bin_repeat_due(int16_t n)
{
    if (n == 0)
        return 1;
    if (n < BIN_REPEAT_DELAY)
        return 0;
    return (n % 3) == 0;
}

/*
 * 0x10cc8
 *
 * **Scroll the parts bin back**, held down.
 *
 * Let go - the button word at 0x5774 is neither 1 nor 2 - and it resets its
 * own repeat phase and hands the screen back to state 0x1000. Held, it moves
 * one page every *third* call: `[0x2632] % 3`, a signed divide by 3 whose
 * remainder is the test, with the counter incremented on every call whether it
 * scrolled or not. That is the auto-repeat, and three frames is its rate.
 *
 * A page back is `bin_part_at_index(-5)`. When that answers where the cursor
 * already is there is nothing before it, and the bin **wraps to the far end**
 * through `bin_scroll_end` rather than stopping. Both moves ask for a redraw
 * by putting 2 in 0x4e93; a move that changes nothing asks for none.
 *
 * 0x4e8b is set to 2 on every path, held or not.
 */
void bin_scroll_back(void)
{
    uint16_t si;

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG2630.word_2632 = 0;
        DG4E67.state = 0x1000;
        DG4E67.redraw_a = 2;
        return;
    }

    if (bin_repeat_due(((int16_t)DG2630.word_2632))) {   /* deviation: see above */
        si = (uint16_t)bin_part_at_index(-5);
        if (si != DG50D3.bin_list_ptr) {
            DG50D3.bin_list_ptr = si;
            DG4E67.redraw_e = 2;
        } else {
            si = bin_scroll_end();
            if (si != DG50D3.bin_list_ptr) {
                DG50D3.bin_list_ptr = si;
                DG4E67.redraw_e = 2;
            }
        }
    }

    DG2630.word_2632++;
    DG4E67.redraw_a = 2;
}

/*
 * 0x10d37
 *
 * **Scroll the parts bin forward**, held down - `bin_scroll_back`'s twin, with
 * its own repeat counter at 0x2634 and the same one-page-in-three rate.
 *
 * The two are not mirror images at the end stop. Back wraps by asking
 * `bin_scroll_end` where the last page is; forward wraps by writing the list
 * head 0x50d7 straight into the cursor, because the head is a constant and the
 * end is not. Forward also does not compare against the old cursor first: a
 * step that answers something sets it, and a step that answers nothing wraps,
 * so 0x4e93 is asked for a redraw either way.
 */
void bin_scroll_forward(void)
{
    uint16_t si;

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG2630.word_2634 = 0;
        DG4E67.state = 0x1000;
        DG4E67.redraw_a = 2;
        return;
    }

    if (bin_repeat_due(((int16_t)DG2630.word_2634))) {   /* deviation: see above */
        si = (uint16_t)bin_part_at_index(5);
        if (si != 0)
            DG50D3.bin_list_ptr = si;
        else
            DG50D3.bin_list_ptr = 0x50d7;
        DG4E67.redraw_e = 2;
    }

    DG2630.word_2634++;
    DG4E67.redraw_a = 2;
}

/*
 * 0x10da9
 *
 * The cursor over the **box above the parts bin** - the region at 576,0 to
 * 632,63, which is row 1 of the table and carries no action bit of its own.
 *
 * Two answers, and it changes the region's *bit* as well as its cursor. While
 * a part is being carried - tool 9 - it hands the cursor question straight to
 * `region_cursor_bin` below, the box beneath it, and sets +0x10 to 0x1000 so a
 * click here does what a click in the bin does. Otherwise the cursor is 0x1a
 * and the bit is 0x2000.
 *
 * So the box is not a fixed control: what it means depends on whether your
 * hand is full. A region's +0x10 is the bit `build_screen_regions` filed from
 * the table, and this is one of the places that rewrites it.
 *
 * The call is the near-to-far thunk - `push si / push cs / call` - so the
 * callee's `retf` finds a full far return; the `nop` between is the assembler
 * padding it, and `pop cx` is the caller clearing its argument.
 */
void region_cursor_bin_above(uint16_t region)
{
    if (DG4E67.word_4e69 == 9) {
        region_cursor_bin(region);
        REGION(region).code = 0x1000;
        return;
    }

    REGION(region).cursor = 0x1a;
    REGION(region).code = 0x2000;
}

/*
 * 0x10dc2
 *
 * **The parts bin's cursor** - the region at 576,100 to 632,144, row 4, whose
 * click handler is 0x10e14 alongside it.
 *
 * Carrying a part, tool 9, the cursor says what is in your hand, by the kind
 * at +4 of the part at DGROUP 0x50d5: a rope, kind 8, gets cursor 8; a belt,
 * kind 0x0a, gets 9; anything else 0. That is the same question
 * `cursor_for_tool` asks for its own ninth tool, asked again here rather than
 * shared, and the pointer is loaded once into DI instead of twice - the two
 * routines are not the same code and are not transcribed as if they were.
 *
 * Otherwise the cursor says whether there is anything here to pick up:
 * `bin_part_at_index` is asked for the region's own +4, and a record answers
 * cursor 2 while nothing answers 0.
 */
void region_cursor_bin(uint16_t region)
{
    if (DG4E67.word_4e69 == 9) {
        uint16_t kind = PART(DG50D3.dragged_part_ptr).kind;

        REGION(region).cursor =
            (kind == 8) ? 8 : (kind == 0x0a) ? 9 : 0;
        return;
    }

    REGION(region).cursor =
        bin_part_at_index((int16_t)REGION(region).word_04) != 0 ? 2 : 0;
}

/*
 * 0x10e14
 *
 * **Clicking the parts bin** - the click handler of the region whose cursor is
 * `region_cursor_bin`, filed at +0x16 of the same row.
 *
 * With a part already in hand it is a bin: the part is thrown away by
 * `discard_carried_part` and the tool is cleared. A rope or a belt sets 0x4e93
 * to 2 on the way, which the discard would set anyway - the original tests the
 * kind twice, here and inside, and both are transcribed.
 *
 * With an empty hand it is a source. `bin_part_at_index` is asked for the
 * region's own +4 and the record it answers is **dereferenced** - the part
 * taken is the one its +0 names, not the entry itself - and nothing there ends
 * the click.
 *
 * Then the interesting part, which is what freeform mode means for the bin.
 * When 0x4e67 is set the part is **cloned** rather than taken, so the bin
 * never empties, and the clone is spliced into the list right after the
 * original: `clone->next = part->next`, that node's `prev` set back to the
 * clone when there is one, `clone->prev = part`, `part->next = clone`.
 *
 * The memory check around it is worth reading slowly. 0x50d5 is set to **0**
 * before `check_room_for_part` and put back afterwards, so the part being
 * picked up is not counted against the room it needs, and the answer decides
 * whether the clone is kept or handed straight back to `free_part`. A refused
 * clone leaves the hand empty and the bin exactly as it was.
 *
 * Whatever ends up in hand, a non-zero 0x50d5 selects tool 9 - which is what
 * makes `cursor_for_tool` and `region_cursor_bin` start answering by kind.
 */
void region_click_bin(uint16_t region)
{
    uint16_t saved;                    /* [bp-2] */
    uint16_t part, clone;

    if (DG4E67.word_4e69 == 9) {
        uint16_t kind = PART(DG50D3.dragged_part_ptr).kind;

        if (kind == 8 || kind == 0x0a)
            DG4E67.redraw_e = 2;

        discard_carried_part();
        DG4E67.word_4e69 = 0;
        return;
    }

    DG4E67.word_4e95 = 0;
    DG4E67.word_4e97 = 0;

    part = DGU16((uint16_t)bin_part_at_index(
                     (int16_t)REGION(region).word_04));
    DG50D3.dragged_part_ptr = part;

    if (part == 0) {
        return;
    }

    if (DG4E67.round_kind != 0) {
        clone = clone_part(PARTP(DG50D3.dragged_part_ptr));
        saved = DG50D3.dragged_part_ptr;
        DG50D3.dragged_part_ptr = 0;

        if (check_room_for_part() != 0) {
            DG50D3.dragged_part_ptr = saved;
            PART(clone).link_ptr = PART(DG50D3.dragged_part_ptr).link_ptr;
            if (PART(clone).link_ptr != 0)
                PART(PART(clone).link_ptr).prev_ptr = clone;
            PART(clone).prev_ptr = DG50D3.dragged_part_ptr;
            PART(DG50D3.dragged_part_ptr).link_ptr = clone;
            DG50D3.dragged_part_ptr = clone;
        } else {
            free_part(PARTP(clone));
        }
    }

    if (DG50D3.dragged_part_ptr != 0) {
        uint16_t kind;

        DG4E67.word_4e69 = 9;
        kind = PART(DG50D3.dragged_part_ptr).kind;
        if (kind == 8 || kind == 0x0a)
            DG4E67.redraw_e = 2;
    }

}

/*
 * 0x10ef1
 *
 * **The playfield's cursor.** A region handler, of the same family as the five
 * at 0x34eb and after: it writes a cursor number into its region's own +0x0e,
 * which `regions_handle_pointer` reads a moment later.
 *
 * Named from the row that installs it rather than from what it does, as the
 * others here had to be. That row is
 *
 *     { 0x4e79, 0, 0x200, { 0x1000, 0, 0, 0, 0x27f, 0x16f, 0, 0x1000,
 *                           0x2f01, 0xdff, 0, 0 } }
 *
 * whose rectangle is 0..0x27f by 0..0x16f - the whole 640-wide picture down to
 * scan line 367, which is exactly where `vm_set_line_compare(0x16f)` splits
 * the screen. So the region is the play area entire, not a button in it, and
 * the cursor it asks for is the one the selected tool wants: the answer comes
 * straight from `cursor_for_tool` at 0x046d8 and is not looked at here.
 *
 * Unlike its five siblings this one has no condition of its own - they choose
 * between two cursors on a flag, and it delegates the whole question.
 */
void region_cursor_playfield(uint16_t region)
{
    REGION(region).cursor = (uint16_t)cursor_for_tool();
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
    /*
     * **The original's `sub sp,0x16` is this routine's own locals**, and the
     * port keeps them in `s` below rather than in DGROUP - so there was
     * nothing left for the reservation to protect. It was kept on the reading
     * that a callee's frame has to land *below* this one; that is true of a
     * routine whose locals are the guest's, and this one's are not. Without
     * it a callee's frame lands 0x18 higher, on bytes nothing reads.
     */
    struct screen_loop s = {0, 0, 0, 0, 0, 0, 0, 0};

    reset_machine();
    paint_game_screen(1);
    set_palette_pointer(DG52ED.pal_tim_ptr.ptr);
    show_cursor_again();

    while (s.done == 0) {
        update_button_state();

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);
        if (((uint8_t)DG52ED.last_key) == 0x0f)
            sub_1156c();

        regions_handle_pointer(DG4E67.regions_panel_ptr);

        if (bit0_of_468c(0x38) && bit0_of_468c(0x2f)) {
            show_message_box(0x1cee, 0x1cfd);   /* "VERSION NUMBER" */
            s.repaint_all = 1;
            DG4E67.state = 2;
        }

        switch (DG4E67.state) {
        case 0x8000:
            paint_panel_a(1);
            present_back_page();
            DG4E67.state = 0x1000;
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
}

/*
0x0f8c2
 *
 * **The game screen's own loop** - where the game sits while a level is being
 * built and run, and the last piece between the briefing and playing.
 *
 * It is a loop on the same DGROUP 0x4e6b that `game_round` dispatches on, so a
 * screen leaves by *writing into that word* rather than by returning: it runs
 * while 0x4e6b is neither 0x2000 nor 2.
 *
 * One pass, in order: clear the two cursor hints to -1, take a key, update the
 * button, scroll the play area, step the counters, offer the key to the music
 * shortcut if this is freeform, let the regions see the pointer, run the bin's
 * arrows if a region asked for them, and then either the pointer frame - if
 * the pointer is in the play area - or the edge-scroll flags if it is not.
 *
 * `si` is the "was outside last frame" latch. It exists so that leaving the
 * play area with a part in hand re-marks that part exactly **once**, on the
 * frame the pointer crosses out, rather than every frame it stays out.
 *
 * **Five deferred redraws** follow, at 0x4e93 down to 0x4e8b, each a countdown
 * and a layer: a change asks for N frames of redraw and gets one a frame. Then
 * the machine is stepped and drawn, the selection decoration goes on if
 * something is selected, and the rubber-band line goes on if a mover asked for
 * one - 0x52c5 is its colour and -1 means no line.
 *
 * **The frame pacing is a spin on 0x44ef**, which the timer counts down: the
 * loop waits until eight have gone by, then reloads 0x2710 and presents. That
 * is the same counter the copy-protection screen reads its page number from,
 * and it is why the two clocks differ there.
 *
 * On the way out, a part still in hand with bit 0x800 in +6 is thrown away if
 * it is a rope or a belt that reached something, and otherwise handed to
 * sub_05482. A rope that never reached anything takes the second path, because
 * the kind test falls through to the belt test and then out.
 */
void game_screen_loop(void)
{
    int16_t si = 0;
    uint16_t part;

    reset_level_state();
    DG44EE.frame_budget = 0x2710;

    while (DG4E67.state != 0x2000 && DG4E67.state != 2) {
        DG52BD.band_colour = 0xffff;
        DG52BD.drop_cursor = 0xffff;

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);

        update_button_state();
        scroll_play_area();
        step_counters();

        if (DG4E67.round_kind != 0)
            select_music_by_key();

        regions_handle_pointer(DG4E67.regions_play_ptr);

        if (DG4E67.state == 0x800)
            bin_scroll_back();
        else if (DG4E67.state == 0x400)
            bin_scroll_forward();

        if (point_in_play_area() != 0) {
            pointer_frame();
            si = 0;
        } else {
            if (DG50D3.dragged_part_ptr != 0 && si == 0) {
                mark_joined_shapes(PARTP(DG50D3.dragged_part_ptr), 3);
                mark_part_shapes(PARTP(DG50D3.dragged_part_ptr), 3);
            }
            edge_scroll_flags();
            si = 1;
        }

        if (DG4E67.redraw_e != 0) { draw_machine_layer_a(); DG4E67.redraw_e--; }
        if (DG4E67.redraw_d != 0) { draw_machine_layer_b(); DG4E67.redraw_d--; }
        if (DG4E67.redraw_c != 0) { draw_machine_layer_c(); DG4E67.redraw_c--; }
        if (DG4E67.redraw_b != 0) { draw_machine_layer_d(); DG4E67.redraw_b--; }
        if (DG4E67.redraw_a != 0) { draw_machine_layer_e(); DG4E67.redraw_a--; }

        mark_parts_in_dirty_rects();
        replay_shapes();
        step_and_draw_machine(0);

        if (DG50D3.dragged_part_ptr != 0 && DG52BD.drop_cursor != -1)
            draw_part_selection(PARTP(DG50D3.dragged_part_ptr), ((uint16_t)DG52BD.drop_cursor), 1);

        if (DG52BD.band_colour != -1) {
            clear_flag_2d44_thunk();
            DG3890.second_colour = ((uint8_t)DG52BD.band_colour);
            clip_and_draw_line(
                (int16_t)(((uint16_t)DG52BD.anchor_x) - ((uint16_t)DG4E67.origin_x)),
                (int16_t)(((uint16_t)DG52BD.anchor_y) - ((uint16_t)DG4E67.origin_y)),
                (int16_t)(((uint16_t)DG52BD.band_x) - ((uint16_t)DG4E67.origin_x)),
                (int16_t)(((uint16_t)DG52BD.band_y) - ((uint16_t)DG4E67.origin_y)));
            restore_cursor_following();
            alloc_shape((const volatile uint8_t *)&DG52BD.anchor_x,
                        (const volatile uint8_t *)&DG52BD.band_x,
                        4, 2, 0);
        }

        if (DG4E67.word_4e89 != 0) { draw_carried_icon(); DG4E67.word_4e89--; }

        seg172c_nothing();

        while ((int16_t)(0x2710 - ((uint16_t)DG44EE.frame_budget)) < 8)
            ;
        DG44EE.frame_budget = 0x2710;

        present_frame(1);
        shift_all_histories();

        if (DG5768.button_right == 2)
            DG4E67.state = 2;
    }

    part = DG50D3.dragged_part_ptr;
    if (part == 0 || (PART(part).flags_06 & 0x800) == 0)
        return;

    if (PART(part).kind == KIND_BELT
        && DGU16((uint16_t)(PART(part).word_54 + 4)) != 0) {
        discard_carried_part();
        return;
    }

    if (PART(part).kind == KIND_ROPE
        && ((uint16_t)BELT(PART(part).word_66).end_a_ptr) != 0) {
        discard_carried_part();
        return;
    }

    sub_05482();
}

/*
 * 0x0faf9
 *
 * **Pick a tune from the keyboard.** DGROUP 0x52f1 is the last key the loop
 * read, *not* a level number, and this is a jump table on it at CS:0x1b8c -
 * the scancode less two, refusing anything past 0x2e.
 *
 * Read as scancodes the sixteen entries are exactly the number row and the
 * first seven letters:
 *
 *     1 2 3 4 5 6 7 8 9   ->  tunes 0x3e9 .. 0x3f1
 *     A B C D E F G       ->  tunes 0x3f2 .. 0x3f8
 *
 * which is why the table looked like an arbitrary jumble of levels - 2..10,
 * then 30, 48, 46, 32, 18, 33, 34 - when read as anything else. The remaining
 * thirty-one entries all point at the arm that loads -1.
 *
 * `game_screen_loop` only calls this when 0x4e67 is set, so the shortcut is a
 * freeform-mode feature and not a cheat that works everywhere.
 *
 * -1 means silence and returns without touching anything. Otherwise the tune
 * is remembered at 0x50bb - which is where `game_round` reads it from when it
 * restarts the music - and started.
 */
void select_music_by_key(void)
{
    static const struct { uint8_t key; int16_t tune; } TUNES[] = {
        {  2, 0x3e9 }, {  3, 0x3ea }, {  4, 0x3eb }, {  5, 0x3ec },
        {  6, 0x3ed }, {  7, 0x3ee }, {  8, 0x3ef }, {  9, 0x3f0 },
        { 10, 0x3f1 }, { 30, 0x3f2 }, { 48, 0x3f3 }, { 46, 0x3f4 },
        { 32, 0x3f5 }, { 18, 0x3f6 }, { 33, 0x3f7 }, { 34, 0x3f8 },
    };
    uint16_t key = ((uint8_t)DG52ED.last_key);
    int16_t si = -1;
    uint16_t i;

    if ((uint16_t)(key - 2) <= 0x2e) {
        for (i = 0; i < sizeof TUNES / sizeof TUNES[0]; i++)
            if (TUNES[i].key == key) {
                si = TUNES[i].tune;
                break;
            }
    }

    if (si == -1)
        return;

    DG50AF.tune = si;
    select_music(DG50AF.tune);
}

/*
 * 0x0fbda
 *
 * Put the level back to the state it starts in: no tool selected, nothing in
 * hand, and the eight words from 0x4e69 and 0x4e87 through 0x4e93 cleared.
 *
 * Those seven at 0x4e87 upward are the loop's **deferred redraw counters** -
 * the ones `game_screen_loop` decrements a frame at a time - so clearing them is
 * cancelling every redraw that was still owed, which is right because the
 * three calls after it redraw everything anyway.
 */
void reset_level_state(void)
{
    DG4E67.word_4e69 = 0;
    DG4E67.word_4e87 = 0;
    DG4E67.word_4e89 = 0;
    DG4E67.redraw_a = 0;
    DG4E67.redraw_b = 0;
    DG4E67.redraw_c = 0;
    DG4E67.redraw_d = 0;
    DG4E67.redraw_e = 0;
    DG50D3.dragged_part_ptr = 0;

    clear_layer_heads();
    reset_machine();
    redraw_machine_area();
}

/*
 * 0x0fd02
 *
 * **Carrying a part off the edge of the play area asks for a redraw.**
 *
 * Only while a part is in hand - tool 9 with something at 0x50d5 - and only
 * for a part that is neither a rope nor a belt, because those two are drawn
 * from their endpoints and do not hang off the pointer.
 *
 * 0x4e89 is set to 1 unconditionally, and then each edge the pointer has gone
 * past sets its own counter to 3: above 8 or below 0x12f in 0x5782, left of 8
 * or right of 0x1ff in 0x5784. The right edge sets two of them, 0x4e93 as well
 * as 0x4e8b.
 *
 * Three, not one, because these are the countdowns `game_screen_loop` works through a
 * frame at a time - the strip has to be repainted for three frames, not
 * redrawn once.
 */
void edge_scroll_flags(void)
{
    uint16_t kind;

    if (DG4E67.word_4e69 != 9 || DG50D3.dragged_part_ptr == 0)
        return;

    kind = PART(DG50D3.dragged_part_ptr).kind;
    if (kind == 8 || kind == 0x0a)
        return;

    DG4E67.word_4e89 = 1;

    if (DG5768.pointer_y < 8)
        DG4E67.redraw_d = 3;
    if (DG5768.pointer_y > 0x12f)
        DG4E67.redraw_c = 3;
    if (DG5768.pointer_x < 8)
        DG4E67.redraw_b = 3;
    if (DG5768.pointer_x > 0x1ff) {
        DG4E67.redraw_e = 3;
        DG4E67.redraw_a = 3;
    }
}

/*
 * 0x0fe84
 *
 * Move a carried **rope** with the pointer, and drop it when the button goes
 * down.
 *
 * Two halves. With the button down, `rope_ends_close` decides what happens:
 * ends too far apart and the rope is thrown away, but only if it had a far
 * part - `di` non-zero - because a rope attached to nothing has nothing to
 * come apart. Close enough, and `find_part_from(0)` is asked what is under the
 * pointer and the rope is joined to it: bit 2 into that part's +8, +0x94
 * refreshed, and the link's **+6 or +4** set depending on whether the far end
 * was already taken. Then the endpoints are recomputed, the rope re-filed, and
 * both the tool and the carried part cleared.
 *
 * With the button up it is only preview: 0x52c1 and 0x52c3 take the far part's
 * anchor - its +0x1e and +0x20 plus the bytes at +0x56 and +0x57 - and 0x52bd
 * and 0x52bf take the pointer in play-area coordinates, so something else can
 * draw the rubber-band line. 0x52c5 is the colour, 0xa when the ends are close
 * enough to join and 0xc when they are not.
 */
void move_carried_rope(void)
{
    uint16_t link = PART(DG50D3.dragged_part_ptr).word_54;
    uint16_t di = DGU16((uint16_t)(link + 4));
    int16_t close = rope_ends_close(link);
    uint16_t si;

    if (DG5768.button_left == 2) {
        if (close == 0) {
            if (di != 0)
                discard_carried_part();
            return;
        }

        si = find_part_from(0);

        if (di != 0) {
            PART(si).flags_08 |= 2;
            PART(si).word_94 = PART(si).flags_08;
            DGU16((uint16_t)(link + 6)) = si;
            PART(si).word_54 = link;

            compute_link_endpoints(link);
            mark_needs_refile(PARTP(DG50D3.dragged_part_ptr), 2);
            refile_part_list(PARTP(DG50D3.dragged_part_ptr));
            DG4E67.word_4e69 = 0;
            DG50D3.dragged_part_ptr = 0;
            return;
        }

        PART(si).flags_08 |= 2;
        PART(si).word_94 = PART(si).flags_08;
        DGU16((uint16_t)(link + 4)) = si;
        PART(si).word_54 = link;
        return;
    }

    if (di == 0)
        return;

    DG52BD.anchor_x = (uint16_t)(((uint16_t)PART(di).pos_x)
                               + PART(di).grab_x);
    DG52BD.anchor_y = (uint16_t)(((uint16_t)PART(di).pos_y)
                               + PART(di).grab_y);
    DG52BD.band_x = (uint16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_x));
    DG52BD.band_y = (uint16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_y));

    DG52BD.band_colour = (close != 0) ? 0x0a : 0x0c;
}

/*
 * 0x0ff80
 *
 * Move a carried **belt** with the pointer, attach it when the button goes
 * down, and preview it when the button is up.
 *
 * `find_belt_anchor` says what is under the pointer and which of its ends;
 * 0x2630 remembers that between frames. Two anchors are refused outright: the
 * one already at the belt's other end (0x5456) and the far part it is already
 * joined to, and both only when there *is* a far part - so the first end can
 * legally land on anything.
 *
 * With the button down and no anchor, a belt that already had a far part is
 * thrown away and one that did not is simply left alone.
 *
 * **The two ends are not symmetric.** The first end - no far part yet - just
 * records itself in the anchor's +0x66 pair and in the link's +2, +6, +0xa and
 * +0xc, and refuses a pulley outright. The second end does the geometry: a
 * pulley anchor takes the belt in its single socket at +0x5a and +0x5e, any
 * other part takes it at the end named by the link's +0xa **and again two
 * slots further on**, then the link is re-measured and the whole thing
 * re-filed and let go of. A pulley on the far side is passed to sub_04d4c
 * either way.
 *
 * Button up is preview only, and it changes the machine anyway when the far
 * end is a pulley: sub_04d4c and three marks, before working out the line to
 * draw. 0x52c1 and 0x52c3 are the anchor point, 0x52bd and 0x52bf the pointer
 * in play-area coordinates, 0x52c5 the colour - 0xa where it would attach and
 * 0xc where it would not.
 */
void move_carried_belt(void)
{
    int16_t far_;                     /* [bp-4] */
    int16_t end;     /* [bp-2] */
    uint16_t si   = PART(DG50D3.dragged_part_ptr).word_66;
    uint16_t di, idx;

    far_ = (int16_t)((uint16_t)BELT(si).end_a_ptr);

    di = find_belt_anchor((volatile uint8_t *)&end, DG2630.word_2630);

    if (di == DG5456.belt_far_end && (uint16_t)far_ != 0)
        di = 0;
    else if (di == (uint16_t)far_ && (uint16_t)far_ != 0)
        di = 0;

    DG2630.word_2630 = di;

    if (DG5768.button_left == 2) {
        if (di == 0) {
            if ((uint16_t)far_ != 0)
                discard_carried_part();
            return;
        }

        if ((uint16_t)far_ == 0) {
            if (PART(di).kind != KIND_PULLEY) {
                PART(di).belt_ptr[(uint16_t)end] = si;
                BELT(si).end_a_ptr = di;
                BELT(si).home_a_ptr = di;
                BELT(si).slot_a = (uint8_t)(uint16_t)end;
                BELT(si).home_slot_a = (uint8_t)(uint16_t)end;
                DG5456.belt_far_end = di;
            }
            return;
        }

        if (PART(DG5456.belt_far_end).kind == KIND_PULLEY) {
            PART(DG5456.belt_far_end).link_right = di;
            PART(DG5456.belt_far_end).link_down = di;
            mark_joined_shapes(PARTP(DG5456.belt_far_end), 3);
            mark_part_shapes(PARTP(DG5456.belt_far_end), 3);
            mark_needs_refile(PARTP(DG5456.belt_far_end), 2);
        } else {
            idx = BELT(si).slot_a;
            PART(DG5456.belt_far_end).link[idx] = di;
            PART(DG5456.belt_far_end).link[idx + 2] = di;
        }

        refresh_link_geometry(si);
        mark_needs_refile(PARTP(DG50D3.dragged_part_ptr), 2);

        if (PART(di).kind == KIND_PULLEY) {
            PART(di).link_left = DG5456.belt_far_end;
            PART(di).link_up = DG5456.belt_far_end;
            PART(di).word_68 = si;
            if (PART(DG5456.belt_far_end).kind == KIND_PULLEY)
                sub_04d4c(PARTP(DG5456.belt_far_end));
            DG5456.belt_far_end = di;
        } else {
            PART(di).link[(uint16_t)end] = DG5456.belt_far_end;
            PART(di).link[(uint16_t)end + 2] = DG5456.belt_far_end;
            PART(di).belt_ptr[(uint16_t)end] = si;
            BELT(si).end_b_ptr = di;
            BELT(si).home_b_ptr = di;
            BELT(si).slot_b = (uint8_t)(uint16_t)end;
            BELT(si).home_slot_b = (uint8_t)(uint16_t)end;
            if (PART(DG5456.belt_far_end).kind == KIND_PULLEY)
                sub_04d4c(PARTP(DG5456.belt_far_end));
            refile_part_list(PARTP(DG50D3.dragged_part_ptr));
            DG4E67.word_4e69 = 0;
            DG50D3.dragged_part_ptr = 0;
        }
        return;
    }

    if ((uint16_t)far_ == 0) {
        return;
    }

    if (PART(DG5456.belt_far_end).kind == KIND_PULLEY) {
        end = (int16_t)1;
        sub_04d4c(PARTP(DG5456.belt_far_end));
        mark_joined_shapes(PARTP(DG5456.belt_far_end), 3);
        mark_part_shapes(PARTP(DG5456.belt_far_end), 3);
        mark_needs_refile(PARTP(DG5456.belt_far_end), 2);
    } else {
        end = (int16_t)BELT(si).slot_a;
    }

    DG52BD.anchor_x = (uint16_t)(((uint16_t)PART(DG5456.belt_far_end).pos_x)
                    + PART(DG5456.belt_far_end).attach[(uint16_t)end].x);
    DG52BD.anchor_y = (uint16_t)(((uint16_t)PART(DG5456.belt_far_end).pos_y)
                    + PART(DG5456.belt_far_end).attach[(uint16_t)end].y);
    DG52BD.band_x = (uint16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_x));
    DG52BD.band_y = (uint16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_y));

    DG52BD.band_colour = (di != 0) ? 0x0a : 0x0c;
}

/*
 * 0x0fc0e
 *
 * **One frame of whatever the pointer is doing to a part** - the level loop's
 * pointer half, and the routine that turns a position into a tool.
 *
 * `si` is set when the hand is already busy: tool 9, or any tool with the top
 * bit, which is a drag in progress. Only when it is *not* busy does this look
 * for something new - `find_part_from` on the currently carried part, with a
 * part whose +6 has bit 0x8000 refused - and only then does
 * `part_handle_at_pointer` choose a tool from where the pointer is.
 *
 * So a drag keeps its tool for as long as it lasts, and a fresh pointer picks
 * one every frame. Nothing under the pointer clears the tool and returns.
 *
 * 0x52c7 is set to 0xa on every frame the hand is not already carrying, which
 * is the plain cursor; the movers overwrite it with 0xe or 0xc when they have
 * an opinion about dropping.
 *
 * The tool then picks an arm through a jump table at CS:0x1cfe, on the tool
 * less one with the top bit stripped, and **anything outside 1 to 10 falls
 * through doing nothing**. Four of the ten act only on the press edge, three
 * act every frame, and tool 10's whole body is to let go of the part.
 */
void pointer_frame(void)
{
    uint16_t si;

    si = (DG4E67.word_4e69 == 9 || (DG4E67.word_4e69 & 0x8000)) ? 1 : 0;

    if (si == 0) {
        DG50D3.dragged_part_ptr = find_part_from(DG50D3.dragged_part_ptr);
        if (DG50D3.dragged_part_ptr != 0
            && (PART(DG50D3.dragged_part_ptr).flags_06 & 0x8000))
            DG50D3.dragged_part_ptr = 0;
    }

    if (DG50D3.dragged_part_ptr == 0) {
        DG4E67.word_4e69 = 0;
        return;
    }

    if (DG4E67.word_4e69 != 9)
        DG52BD.drop_cursor = 0x0a;

    if (si == 0)
        DG4E67.word_4e69 = part_handle_at_pointer(PARTP(DG50D3.dragged_part_ptr));

    switch ((uint16_t)((DG4E67.word_4e69 & 0x7fff) - 1)) {
    case 0:                                     /* tool 1 */
        if (DG5768.button_left == 2)
            flip_carried_end_1();
        return;
    case 1:                                     /* tool 2 */
        if (DG5768.button_left == 2)
            flip_carried_end_2();
        return;
    case 2: case 3: case 4: case 5:             /* tools 3 to 6 */
        run_drag_frame();
        return;
    case 6:                                     /* tool 7 */
        if (DG5768.button_left == 2)
            pick_up_part();
        return;
    case 7:                                     /* tool 8 */
        if (DG5768.button_left == 2)
            discard_carried_part();
        return;
    case 8:                                     /* tool 9 */
        move_carried();
        return;
    case 9:                                     /* tool 10 */
        if (DG5768.button_left == 2)
            DG50D3.dragged_part_ptr = 0;
        return;
    default:
        return;
    }
}

/*
 * 0x0fd65
 *
 * **Scroll the play area** when the pointer is against an edge, and re-file
 * everything in it if it moved.
 *
 * The current origins at 0x4ea3 and 0x4ea1 are first copied down to 0x4e9b and
 * 0x4e99, which is where `draw_carried_icon` reads the *previous* position
 * from - so this is also what makes an icon's backdrop restorable after a
 * scroll.
 *
 * Then four edges, each a pair of tests: the pointer at or past the edge, and
 * the origin not already at its stop. Left stops at -8 and top at -8; right
 * and bottom stop at 0x50b7 and 0x50b9, which is where the level's own extent
 * is kept. A step is 0x10 either way. **Both axes can move in one call** - the
 * flag is shared and the two offsets are independent - so a pointer held in a
 * corner scrolls diagonally.
 *
 * Nothing is written back unless something moved. When it did, every part is
 * walked - `pick_by_flag(0x3000)` then `pick_for_record(si, 0x1000)` - and
 * each one that does not have bit 0x2000 in +8 is marked for re-filing and its
 * shapes re-marked. The parts do not move; the window over them does, so what
 * was drawn where is no longer true.
 *
 * The origins are stored **after** that walk, not before, so the marking sees
 * the old position.
 */
void scroll_play_area(void)
{
    uint16_t di, y, moved = 0, si;

    DG4E67.origin_c_x = ((uint16_t)DG4E67.origin_b_x);
    DG4E67.origin_c_y = ((uint16_t)DG4E67.origin_b_y);
    DG4E67.origin_b_x = ((uint16_t)DG4E67.origin_x);
    DG4E67.origin_b_y = ((uint16_t)DG4E67.origin_y);

    di = ((uint16_t)DG4E67.origin_x);
    y = ((uint16_t)DG4E67.origin_y);

    if ((int16_t)((uint16_t)DG5768.pointer_x) <= 0 && DG4E67.origin_x != -8) {
        moved = 1;
        di = (uint16_t)(di - 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_x) >= 0x27f && ((uint16_t)DG4E67.origin_x) != ((uint16_t)DG50AF.extent_y)) {
        moved = 1;
        di = (uint16_t)(di + 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_y) <= 0 && DG4E67.origin_y != -8) {
        moved = 1;
        y = (uint16_t)(y - 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_y) >= 0x16f && ((uint16_t)DG4E67.origin_y) != ((uint16_t)DG50AF.extent_x)) {
        moved = 1;
        y = (uint16_t)(y + 0x10);
    }

    if (moved == 0)
        return;

    si = pick_by_flag(0x3000);
    while (si != 0) {
        if ((PART(si).flags_08 & 0x2000) == 0) {
            mark_needs_refile(PARTP(si), 2);
            mark_part_shapes(PARTP(si), 3);
        }
        si = pick_for_record(si, 0x1000);
    }

    DG4E67.origin_x = di;
    DG4E67.origin_y = y;
}

/*
 * 0x1295f
 *
 * **Is this file one of ours?** It opens the name, reads one word, and answers
 * whether that word is **0xaced** - the machine file's magic, and the only
 * check made before the loader is trusted with the rest.
 *
 * Both exits close the file, and the failure exit closes it *even when the open
 * failed*, handing `fclose` the zero it just tested. That is what the original
 * does; the runtime's `fclose` looks the pointer up rather than following it,
 * so it is a wasted call rather than a fault.
 */
uint16_t is_machine_file(uint16_t name)
{
    int16_t magic;                /* [bp-2] */
    uint16_t file;
    uint16_t ok    = 0;

    file = game_fopen(dg_ptr(dgroup, name), dg_ptr(dgroup, 0x2884 /* "rb" */));

    if (file != 0) {
        game_fread_far(file, (volatile uint8_t *)&magic);
        if ((uint16_t)magic == 0xaced)
            ok = 1;
    }

    game_fclose(file);
    return ok;
}

/*
 * 0x12a2f
 *
 * **A puzzle's title, out of its own level file.** The name is built rather
 * than looked up - `"l"`, the number, `".lev"` - so puzzle 7 is `l7.lev` and
 * there is no table anywhere saying so.
 *
 * The file is checked with the same 0xaced `is_machine_file` looks for, and
 * then **one word is read and thrown away** before the title. Nothing here says
 * what it is; the title is what follows it.
 *
 * A missing file, or a wrong magic, answers 0 - which is what stops the list
 * drawer, so the number of puzzles is however many files are actually there.
 */
uint16_t get_puzzle_title(int16_t n, volatile uint8_t * buf)
{
    uint8_t name[14];                 /* [bp-0x1a] */
    uint8_t num[8]; /* [bp-0x0c] */
    uint8_t skip[2]; /* [bp-4]    */
    int16_t magic; /* [bp-2]   */
    uint16_t file;
    uint16_t ok = 0;

    string_copy((volatile uint8_t *)name, dg_ptr(dgroup, 0x2891 /* "l" */));
    int_to_string(n, (volatile uint8_t *)num, 10);
    string_concat((volatile uint8_t *)name, (volatile uint8_t *)num);
    string_concat((volatile uint8_t *)name, dg_ptr(dgroup, 0x2893 /* ".lev" */));

    file = game_fopen((volatile uint8_t *)name, dg_ptr(dgroup, 0x2898 /* "rb" */));

    if (file != 0) {
        game_fread_far(file, (volatile uint8_t *)&magic);

        if ((uint16_t)magic != 0xaced) {
            game_fclose(file);
        } else {
            game_fread_far(file, (volatile uint8_t *)skip);
            game_fread_string(file, buf);
            game_fclose(file);
            ok = 1;
        }
    }
    return ok;
}

/*
 * 0x12ad0
 *
 * **A password into a level number**, by finding it in `password.txt`.
 *
 * The text is upper-cased in place first, and then **cut at the first `-`** -
 * a NUL is written over it - so a code of the form `WORD-SCORE` matches on the
 * word alone. The dash is put back before the routine answers, because the same
 * buffer is about to be handed to the score decoder, which wants the half this
 * one just hid.
 *
 * The line counter starts at **1 and is incremented before the comparison**, so
 * a match on the file's first line answers 2. Whether that is deliberate or an
 * off-by-one cannot be told from here - it is consistent, so a password file
 * written to suit it works.
 *
 * The loop cannot tell a blank line from the end of the file, because
 * `game_fread_line` reports both as an empty buffer.
 *
 * Not found is 0xffff, and a file that will not open leaves it at that without
 * reading anything.
 */
uint16_t password_to_level(uint16_t text)
{
    uint8_t line[26];                    /* [bp-0x1a] */
    volatile uint8_t *  dash;
    uint16_t file;
    int16_t  n      = 1;                    /* [bp-4] */
    int16_t  answer = -1;                   /* [bp-2] */

    string_upper(dg_ptr(dgroup, text));

    dash = string_chr(dg_ptr(dgroup, text), '-');
    if (dash != NULL)
        *dash = 0;

    file = game_fopen(dg_ptr(dgroup, 0x289b /* "password.txt" */), dg_ptr(dgroup, 0x28a8 /* "rb" */));

    if (file != 0) {
        game_fread_line(file, (volatile uint8_t *)line);

        while ((*line) != 0) {
            n++;

            if (string_compare_nocase(dg_ptr(dgroup, text),
                                      (volatile uint8_t *)line) == 0)
                answer = n;

            game_fread_line(file, (volatile uint8_t *)line);
        }

        game_fclose(file);
    }

    if (dash != NULL)
        *dash = '-';
    return (uint16_t)answer;
}

/*
 * 0x12bed
 *
 * **Writes `tim.cfg`** - the whole of the game's saved state between runs, and
 * it is two words: the furthest level reached at DGROUP 0x4eb7 and the sound
 * level at 0x4ec1. Nothing else survives quitting.
 *
 * It writes them with `write_word`, the same routine the machine files use, so
 * the file's four bytes are in the same byte order as everything else the game
 * writes. A failed open is silently nothing - the settings just do not persist.
 */
void sub_12bed(void)
{
    uint16_t file = game_fopen(dg_ptr(dgroup, 0x28c6 /* "tim.cfg" */), dg_ptr(dgroup, 0x28ce /* "wb" */));

    if (file != 0) {
        write_word(file, dg_ptr(dgroup, 0x4eb7));
        write_word(file, dg_ptr(dgroup, 0x4ec1));
        game_fclose(file);
    }
}

/*
 * 0x0efdc
 *
 * Give back the two bitmap lists the game keeps at DGROUP 0x4ecd and 0x4ec9,
 * in that order, through the driver's own thunk.
 */
void free_two_bitmap_lists(void)
{
    free_bitmaps_thunk(BMPLIST(DG4E67.score2_bmp_ptr));
    free_bitmaps_thunk(BMPLIST(DG4E67.menu_bmp_ptr));
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
    uint8_t name[14];            /* [bp-0x16] */
    uint8_t number[8];          /* [bp-8]    */

    string_copy(name, dg_ptr(dgroup, 0x2625));               /* "part" */
    int_to_string((int16_t)n, number, 10);
    string_concat(name, number);
    string_concat(name, dg_ptr(dgroup, 0x262a));             /* ".bmp" */

    heap_check_or_hang();
    clear_flag_2d44_thunk();

    PARTKIND(n).bitmaps_ptr = load_bitmaps(name);

    restore_cursor_following();
    heap_check_or_hang();
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

    free_bitmaps_thunk(BMPLIST(DGU16(at)));
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
    int16_t si;

    DG546C.table = dos_alloc_bytes((uint16_t)(n * 4), 0, 0).ptr;

    for (si = 0; si < n; si++)
        FARU16(DG546C.table.seg, (uint16_t)(DG546C.table.off + 2 * si)) =
            heap_calloc_far(1, 0xa2);
}

/*
 * 0x11db4
 *
 * Read one byte: `game_fread(buf, 1, 1, file)`, with the file first and the
 * buffer second - the same order round as `game_fread_far` beside it.
 *
 * It **answers what `fread` answered**, falling through with it in AX rather
 * than discarding it, which is how `read_line` below tells an empty line from
 * the end of the file.
 */
uint16_t game_fread_byte(uint16_t file, volatile uint8_t * buf)
{
    return game_fread(buf, 1, 1, file);
}

/*
 * 0x11e0b
 *
 * **Read one line.** Bytes into the buffer until a `\n` is seen, and then the
 * terminator goes at **`[si - 1]`** - over the byte *before* the newline, not
 * over the newline. That is not an off-by-one: the file has DOS line endings,
 * so the byte before the `\n` is the `\r`, and one store removes both.
 *
 * A file with Unix endings would therefore lose the last character of every
 * line. Nothing here checks.
 *
 * The very first read is the only one whose answer is looked at, and a zero
 * there - end of file - writes an empty string. So a caller loops until the
 * line comes back empty, and cannot tell that from a blank line in the file.
 * A blank line is also where the `[si - 1]` store writes one byte *below* the
 * buffer, because there is no `\r` in front of the `\n` to absorb it.
 */
void game_fread_line(uint16_t file, volatile uint8_t * buf)
{
    volatile uint8_t * si = buf;

    if (game_fread_byte(file, si) == 0) {
        *si = 0;
        return;
    }

    while (*si != '\n') {
        si++;
        game_fread_byte(file, si);
    }

    si[-1] = 0;
}

/*
 * 0x11dd1
 *
 * A far-callable two-byte read: `game_fread(buf, 2, 1, file)`, with the
 * arguments the other way round from `fread`'s own - the file first and the
 * buffer second.
 */
void game_fread_far(uint16_t file, volatile uint8_t * buf)
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
void game_fread_string(uint16_t file, volatile uint8_t * buf)
{
    for (;;) {
        game_fread_byte(file, buf);
        if (*buf == 0)
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
void read_record_fields(uint16_t file, struct part *rec)
{
    int16_t v10;       /* [bp-0x10] */
    uint8_t v0b;                  /* [bp-0x0b] */
    int16_t v0a;       /* [bp-0x0a] */
    int16_t v08;       /* [bp-8] */
    int16_t v06;       /* [bp-6] */
    int16_t v04;       /* [bp-4] */
    int16_t v02;       /* [bp-2] */
    uint16_t di;

    game_fread_far(file, (volatile uint8_t *)&rec->kind);
    game_fread_far(file, (volatile uint8_t *)&rec->flags_06);
    game_fread_far(file, (volatile uint8_t *)&rec->word_94);
    rec->flags_08 = rec->word_94;

    if (DG546C.version >= 0x101)
        game_fread_far(file, (volatile uint8_t *)&rec->flags_0a);

    game_fread_far(file, (volatile uint8_t *)&rec->word_90);
    rec->form = rec->word_90;

    game_fread_far(file, (volatile uint8_t *)&rec->word_92);
    rec->direction = rec->word_92;

    game_fread_far(file, (volatile uint8_t *)&rec->width);
    game_fread_far(file, (volatile uint8_t *)&rec->height);
    rec->word_42 = ((uint16_t)rec->height);
    rec->word_40 = ((uint16_t)rec->width);

    game_fread_far(file, (volatile uint8_t *)&rec->word_50);
    game_fread_far(file, (volatile uint8_t *)&rec->word_52);
    game_fread_far(file, (volatile uint8_t *)&rec->word_8c);
    game_fread_far(file, (volatile uint8_t *)&rec->word_8e);
    game_fread_far(file, (volatile uint8_t *)&rec->word_96);

    game_fread_far(file, (volatile uint8_t *)&v02);
    game_fread_byte(file, (volatile uint8_t *)&rec->grab_x);
    game_fread_byte(file, (volatile uint8_t *)&rec->grab_y);
    game_fread_far(file, (volatile uint8_t *)&rec->word_58);

    if (v02 != 0) {
        uint16_t rope = heap_calloc_far(1, 0x38);   /* [bp-0x0e] */

        rec->word_54 = rope;
        ROPE(rope).owner_ptr = dg_off(dgroup, rec);

        game_fread_far(file, (volatile uint8_t *)&v06);
        ROPE(rope).end_a_ptr =
            (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);

        game_fread_far(file, (volatile uint8_t *)&v06);
        ROPE(rope).end_b_ptr =
            (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);

        if (ROPE(rope).end_a_ptr != 0)
            PART(ROPE(rope).end_a_ptr).word_54 = rope;

        if (ROPE(rope).end_b_ptr != 0)
            PART(ROPE(rope).end_b_ptr).word_54 = rope;
    }

    for (v0a = (int16_t)0; v0a < 2; v0a++) {
        game_fread_far(file, (volatile uint8_t *)&v04);
        game_fread_byte(file, (volatile uint8_t *)&rec->attach[(uint16_t)v0a].x);
        game_fread_byte(file, (volatile uint8_t *)&rec->attach[(uint16_t)v0a].y);

        if (v04 == 0)
            continue;

        di = heap_calloc_far(1, 0x2c);
        rec->belt_ptr[(uint16_t)v0a] = di;
        BELT(rec->belt_ptr[(uint16_t)v0a]).owner_ptr = dg_off(dgroup, rec);

        game_fread_far(file, (volatile uint8_t *)&v06);
        BELT(di).end_a_ptr =
            (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);
        BELT(di).home_a_ptr = BELT(di).end_a_ptr;

        game_fread_far(file, (volatile uint8_t *)&v06);
        BELT(di).end_b_ptr =
            (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);
        BELT(di).home_b_ptr = BELT(di).end_b_ptr;

        game_fread_byte(file, &BELT(di).slot_a);
        BELT(di).home_slot_a = ((int8_t)BELT(di).slot_a);
        game_fread_byte(file, &BELT(di).slot_b);
        BELT(di).home_slot_b = ((int8_t)BELT(di).slot_b);

        if (BELT(di).end_a_ptr != 0)
            PART(BELT(di).end_a_ptr).belt_ptr[(int8_t)BELT(di).slot_a] = di;

        if (BELT(di).end_b_ptr != 0)
            PART(BELT(di).end_b_ptr).belt_ptr[(int8_t)BELT(di).slot_b] = di;
    }

    for (v0a = (int16_t)0; v0a < 2; v0a++) {
        game_fread_far(file, (volatile uint8_t *)&v06);
        rec->link[(uint16_t)v0a + 2] =
            (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);
        rec->link[(uint16_t)v0a] =
            rec->link[(uint16_t)v0a + 2];
    }

    if (DG546C.version >= 0x101) {
        for (v0a = (int16_t)4; v0a < 6; v0a++) {
            game_fread_far(file, (volatile uint8_t *)&v06);
            rec->link[(uint16_t)v0a] =
                (uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);
        }
    }

    if (rec->kind == KIND_PULLEY) {
        game_fread_far(file, (volatile uint8_t *)&v06);
        v10 = (int16_t)(uint16_t)lookup_table_546c((int16_t)(uint16_t)v06);
        if ((uint16_t)v10 != 0)
            rec->word_68 =
                PART((uint16_t)v10).word_66;
    }

    if (DG546C.version <= 0x101) {
        game_fread_far(file, (volatile uint8_t *)&v08);
        if (v08 != 0) {
            for (v0a = (int16_t)0; v0a < v08; v0a++) {
                game_fread_byte(file, &v0b);
                game_fread_byte(file, &v0b);
            }
        }
    }

    rec->point_count = PARTKIND(rec->kind).point_count;

    if (rec->point_count != 0)
        rec->points_ptr =
            heap_calloc_far(rec->point_count, 4);

    call_part_setup(PARTKIND(rec->kind).setup, dg_off(dgroup, rec));
}

/*
 * 0x1221b
 *
 * Read `n` things out of the file and put them on a list.
 *
 * DGROUP 0x5470 counts them, and each one's number is turned into its record by
 * `lookup_table_546c` before being read into - so the records were made in
 * advance by `alloc_part_table` and this only fills them. `insert_sorted` puts
 * each on the list the caller named.
 *
 * **Only two of the three lists are sorted at all.** `insert_sorted` knows
 * 0x50d7 and 0x5179 and compares nothing for any other head, inserting at the
 * front - and the machine's own parts go on **0x521b**. So that list comes back
 * in exactly the *reverse* of the order the file holds, and writing it head
 * first emits the reverse again.
 *
 * That is measured, not inferred: a machine loaded and saved differs from the
 * file it came from in 280 of 740 bytes, and walking both by the record flags
 * gives kinds `15 39 2 5 2 2 2 50 21 1 1 3 8` in the file against
 * `8 3 1 1 21 50 2 2 2 5 2 39 15` in the save - the same parts, exactly turned
 * around. An earlier version of this comment said the lists "come out in the
 * order the file's contents demand", which is true of the two sorted ones and
 * false of this one.
 *
 * The list head is cleared first, both words of it.
 */
void read_list(uint16_t file, uint16_t head, int16_t n)
{
    int16_t di;

    DGU16((uint16_t)(head + 2)) = 0;
    DGU16(head) = 0;

    for (di = 0; di < n; di++) {
        uint16_t rec = (uint16_t)lookup_table_546c((int16_t)DG546C.record_count);

        read_record_fields(file, PARTP(rec));
        insert_sorted(PARTP(rec), head);
        DG546C.record_count++;
    }
}


/*
 * 0x12c26
 *
 * **The file picker**, and a whole screen with its own loop. It answers 1 when
 * the player chose a file, leaving the name at DGROUP 0x52fe where
 * `load_animation` is then given it.
 *
 * **The mode word 0x4e6b is the whole of the state machine**, exactly as it is
 * in `game_screen`: the regions the pointer is over write it, this loop reads
 * it, and the jump table at 0x1318d dispatches on it. The picker borrows the
 * word, keeping what it displaced at 0x568f - which is also how every drawing
 * routine below knows whether it is a LOAD or a SAVE without being told.
 *
 * **`was`, the previous pass's mode, is what makes the text fields work.** A
 * field commits when it *stops* having focus, and by then 0x4e6b no longer says
 * so - the only place the old value survives is this local, taken at the bottom
 * of the loop. So both field blocks are entered when either the current or the
 * previous mode is theirs.
 *
 * **A full repaint suppresses the partial ones.** The five counters below are
 * not cleared by it; they are simply not acted on in a pass that repainted
 * everything, and stay pending for the next one. Drawing them again over a
 * fresh screen would be redundant, and the counters exist so that a redraw
 * asked for while a message box was up is not lost.
 *
 * Two of its calls are into `dos_chdir` and `dos_setdisk`, which are stubs in
 * this port for the reason given at each: there is nowhere to change to. So the
 * picker draws, scrolls and types, and reaches a stub the moment a directory is
 * actually chosen.
 */
uint16_t pick_file(uint16_t arg1, uint16_t arg2, uint16_t pattern)
{
    uint8_t pat[38];                  /* [bp-0x26], 0x26 bytes */

    int16_t  reload    = 2;             /* [bp-6]    */
    int16_t  idx       = 0;             /* [bp-0xa]  */
    int16_t  rp_up     = 0;             /* [bp-0xc]  */
    int16_t  rp_down   = 0;             /* [bp-0xe]  */
    int16_t  rp_list   = 0;             /* [bp-0x10] */
    int16_t  rp_file   = 0;             /* [bp-0x12] */
    int16_t  rp_name   = 0;             /* [bp-0x14] */
    int16_t  valid;                     /* [bp-0x16] */
    int16_t  repaint   = 0;             /* si */
    uint16_t was       = 0x8000;        /* di */
    uint16_t answer;

    /*
     * **The pattern is the third argument, and it is copied.** `sub_13a8a`
     * takes it apart to build the extension filter and `string_chr` walks it in
     * place, so what the listing filters on is this copy and never the caller's
     * constant.
     */
    string_copy((volatile uint8_t *)pat, dg_ptr(dgroup, pattern));

    DG4E4E.name_buf[0] = 0;
    DG568F.picker_mode = DG4E67.state;
    DG4E67.state = 0x8000;

    for (;;) {
        if (reload != 0) {
            picker_begin(arg1, arg2, (volatile uint8_t *)pat);

            if (((uint16_t)DG568F.word_569d) == 0) {
                answer = 0;
                goto out;
            }

            reload  = 0;
            repaint = 1;
        }

        update_button_state();
        DG52ED.last_key = (uint8_t)bios_read_key();

        if (((uint8_t)DG52ED.last_key) == 9 && DG4E67.state != 0x4000
            && DG4E67.state != 0x1000)
            picker_tab();

        if ((((uint8_t)DG52ED.last_key) == 0x0d || ((uint8_t)DG52ED.last_key) == 0x20
             || ((uint8_t)DG52ED.last_key) == 0x1b)
            && DG4E67.state == 0x4000)
            DG5768.button_left = 0;

        regions_handle_pointer(DG4E67.regions_c_ptr);

        if (DG4E67.state == 0x100)
            goto dispatch;

        /*
         * **The path field.** It is entered while the field has focus *or* had
         * it on the pass before - `was` is last pass's 0x4e6b, taken at the
         * bottom of the loop - because losing focus is what commits the typed
         * path, and by then 0x4e6b no longer says the field.
         */
        if (DG4E67.state == 0x4000 || was == 0x4000) {
            DG4E67.file_op_active = 1;

            if ((((uint8_t)DG52ED.last_key) == 0x0d || DG4E67.state != 0x4000)
                && was == 0x4000) {
                /*
                 * A path of exactly `X:` skips the first `chdir` and goes
                 * straight to the second. Every other path is handed to
                 * `chdir` **twice** - once to find out whether it is reachable
                 * and once to go there - and the drive is selected only after
                 * the second succeeds.
                 */
                if ((DG53AB.byte_53ac == ':' && DG53AB.byte_53ad == 0)
                    || dos_chdir(dg_off(dgroup, DG530B.path_field)) == 0) {
                    if (dos_chdir(dg_off(dgroup, DG530B.path_field)) == 0) {
                        dos_setdisk(DG53AB.word_53ab);
                        reload = 2;
                    } else {
                        dos_get_cur_dir(dg_off(dgroup, DG530B.path_field));
                        show_message_box(0x204f /* "PATH ERROR" */,
                                         0x205a);
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        rp_name = 2;
                    }

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                } else {
                    dos_get_cur_dir(dg_off(dgroup, DG530B.path_field));
                    show_message_box(0x204f /* "PATH ERROR" */, 0x205a);
                    wait_cursor();
                    paint_panel_frame();
                    restore_cursor();
                    repaint = 1;
                    rp_name = 2;

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                }
            } else {
                if (was == 0x4000)
                    picker_type(((uint8_t)DG52ED.last_key),
                        dg_off(dgroup, DG530B.path_field), 0x50);

                rp_name = 2;
            }

            DG4E67.file_op_active = 0;
        }

        /*
         * The name field, the same shape and a different buffer. The original
         * tests the mode **twice** on the way in - once for each half of the
         * `||` - and the second test can never fail once the first has let it
         * through, so the branch it guards is unreachable and is not written
         * out here.
         */
        if (DG4E67.state == 0x1000 || was == 0x1000) {
            if ((((uint8_t)DG52ED.last_key) == 0x0d || DG4E67.state != 0x1000)
                && was == 0x1000) {
                force_extension(dg_off(dgroup, DG4E4E.name_buf), 0x2918 /* "TIM" */);

                if (DG4E67.state == 0x1000)
                    DG4E67.state = 0x8000;
            } else if (was == 0x1000) {
                picker_type(((uint8_t)DG52ED.last_key), dg_off(dgroup, DG4E4E.name_buf),
                            sizeof DG4E4E.name_buf);
            }

            rp_file = 2;
        }

    dispatch:
        switch (DG4E67.state) {
        case 0x0800:                    /* the up arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(DG568F.scroll - 1);

                if (v >= 0) {
                    DG568F.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_up = 2;
            break;

        case 0x0400:                    /* the down arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(DG568F.scroll + 1);

                if ((int16_t)(DG568F.entry_count - 12) >= v) {
                    DG568F.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_down = 2;
            break;

        case 0x2000: {                  /* a click in the listing */
            struct far_ptr rec;

            /*
             * **Which row was clicked is arithmetic, not a hit test.** The
             * pointer's y at 0x5782 less the box's top, divided by the ten
             * pixels a row takes, plus the scroll position.
             */
            idx = (int16_t)((int16_t)(DG5768.pointer_y - 0x7c) / 10
                            + DG568F.scroll);

            if (idx >= DG568F.entry_count) {
                DG4E67.state = 0x8000;
                break;
            }

            /* Entry `idx` of the array at the block's front. */
            rec = ((struct far_ptr far *)MK_FP(DG568F.block.seg,
                                               DG568F.block.off))[idx];

            if (FAR8(rec.seg, rec.off) != ':'
                && FAR8(rec.seg, rec.off) != '<') {
                string_copy((volatile uint8_t *)DG4E4E.name_buf,
                            dg_ptr(dgroup,
                                   listing_to_name((const char far *)MK_FP(rec.seg, rec.off))));
                rp_file = 2;
                DG4E67.state = 0x8000;
                break;
            }

            /*
             * Row zero is the `:` when there is one, and the only way to tell
             * it from a directory called nothing is that we are not at a root.
             */
            if (idx != 0 || path_is_root(dg_off(dgroup, DG530B.path_field)) != 0)
                path_join(dg_off(dgroup, DG530B.path_field),
                          (const char far *)MK_FP(rec.seg, rec.off));
            else
                path_up(dg_off(dgroup, DG530B.path_field));

            DG4E67.file_op_active = 1;

            if (dos_chdir(dg_off(dgroup, DG530B.path_field)) == 0)
                dos_setdisk(DG53AB.word_53ab);

            DG4E67.file_op_active = 0;
            reload = 2;
            DG4E4E.name_buf[0] = 0;
            DG4E67.state = 0x8000;
            break;
        }

        case 0x0200:                    /* the LOAD or SAVE button */
            valid = (int16_t)validate_filename();

            if (valid == 0) {
                picker_draw_action();
                show_message_box(0x1fa0 /* "FILE ERROR" */,
                                 ((uint16_t)DG568F.picker_mode) == 0x100 ? 0x1fd0 : 0x1fab);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            } else if (((uint16_t)DG568F.picker_mode) == 0x80) {
                if (valid == 2) {
                    picker_draw_action();

                    if (ask_yes_no(0x1f5e /* "OVERWRITE FILE" */, 0x1f6d)
                        == 0) {
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        DG4E67.state = 0x8000;
                    }
                }
            } else if (is_machine_file(dg_off(dgroup, DG4E4E.name_buf)) == 0) {
                picker_draw_action();
                show_message_box(0x2076 /* "WRONG FORMAT" */, 0x2083);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            }
            break;

        default:
            break;
        }

        was = DG4E67.state;

        /*
         * **A whole repaint is not one of the partial ones.** When it happens
         * every pending partial redraw is left pending and the pass ends - the
         * full paint has already drawn all of them, and running them again
         * would draw over what it just put down.
         */
        if (repaint != 0) {
            picker_repaint();
            repaint--;
        } else {
            if (rp_up != 0) {
                picker_draw_up();
                rp_up--;
            }
            if (rp_down != 0) {
                picker_draw_down();
                rp_down--;
            }
            if (rp_list != 0) {
                picker_draw_list();
                rp_list--;
            }
            if (rp_name != 0) {
                picker_draw_name();
                rp_name--;
            }
            if (rp_file != 0) {
                picker_draw_filename();
                rp_file--;
            }

            present_frame(1);
        }

        if (DG4E67.state == 0x200 || DG4E67.state == 0x100)
            break;
    }

    /*
     * The listing block goes back **only when it is not the borrowed one**:
     * `picker_begin` will take the pointer at 0x3576 if there is one, and
     * freeing that would hand back memory the picker never owned.
     */
    if (DG568F.block.off != DG3576.scratch.off || DG568F.block.seg != DG3576.scratch.seg) {
        dos_free_far(DG568F.block);
        DG568F.block.seg = 0;
        DG568F.block.off = 0;
        DG568F.word_5697 = 0;
        DG568F.entry_size = 0;
    }

    picker_draw_action();

    if (DG4E67.state == 0x200 && string_length((const volatile uint8_t *)DG4E4E.name_buf) != 0) {
        string_copy((volatile uint8_t *)DG52FE.name, (const volatile uint8_t *)DG4E4E.name_buf);
        answer = 1;
    } else {
        DG4E4E.name_buf[0] = 0;
        answer = 0;
    }

out:
    return answer;
}

/*
 * 0x13d75
 *
 * **A listing record back into a plain name.** The record is far and the answer
 * is near - DGROUP 0x5682, one shared buffer - so the caller gets something it
 * can hand to `strcpy` without carrying a segment around.
 *
 * It strips exactly three things: `<`, `>` and spaces. That undoes both of the
 * shapes `sub_13a8a` writes, the angle brackets round a directory and the
 * padding that lines the extensions up, with one filter rather than two.
 *
 * A `:` record does not go through the loop at all; it answers the constant
 * `".."`, so the way back up leaves here as a path DOS understands rather than
 * as the marker the listing keeps it as.
 */
uint16_t listing_to_name(const char far * entry)
{
    uint16_t si;

    if (*entry == ':')
        return 0x2963;                  /* ".." */

    si = dg_off(dgroup, DG5682.name);

    while (*entry != 0) {
        uint8_t c = *entry;

        if (c != '<' && c != '>' && c != ' ') {
            DG8(si) = c;
            si++;
        }
        entry++;
    }

    DG8(si) = 0;
    return dg_off(dgroup, DG5682.name);
}

/*
 * 0x139ac
 *
 * **Draw the listing.** Twelve rows of ten pixels in a 0x70 by 0x80 box at
 * (0x40, 0x78), text transparent and white, and the far pointers walked in
 * order - so what the sort did is what shows.
 *
 * **This is where the `:` record gets its words.** A row whose text begins with
 * a colon is drawn as *"&lt;PARENT DIR&gt;"* instead: the pointer is swapped for one
 * into DGROUP and the row draws normally. So the listing holds a one-byte
 * marker and the screen holds a phrase, and nothing in between has to know both.
 *
 * The scroll position at 0x5691 is **clamped on the way in, not on the way
 * out**: a listing of twelve or fewer starts at zero whatever 0x5691 says, and
 * one that has scrolled past the end is pulled back to `count - 12`. The
 * comparison is `>`, against the count rather than against `count - 12`, so a
 * position exactly at the count is *kept* and only one past it is caught.
 *
 * Two conditions end the loop, the count and the room left, and the room is
 * counted down in the same 0x0a steps the rows are drawn in.
 */
void picker_draw_list(void)
{
    int16_t  x = 0x40;                  /* [bp-0xa] */
    int16_t  y = 0x78;                  /* di */
    int16_t  w = 0x70;                  /* [bp-0xc] */
    int16_t  room = 0x80;               /* [bp-0xe] */
    /* The block is an array of far pointers, one per entry. */
    struct far_ptr far *p;              /* [bp-4], [bp-2] */
    int16_t  top, i;

    fill_panel_area(x, y, w, room, 0);

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.unknown_02 = 1;                    /* transparent text */
    DG3890.unknown_00 = 0x0f;

    if (DG568F.entry_count > 0x0c) {
        top = DG568F.scroll;
        if (top > DG568F.entry_count)
            top = (int16_t)(DG568F.entry_count - 12);
    } else {
        top = 0;
    }

    p = (struct far_ptr far *)MK_FP(DG568F.block.seg, DG568F.block.off);
    p += top;                           /* skip the rows scrolled past */

    i = 0;
    while (i < DG568F.entry_count && room >= 0x0a) {
        struct far_ptr t = *p++;

        if (FAR8(t.seg, t.off) == ':')
            t = (struct far_ptr){ 0x2191, DGROUP_SEG }; /* "<PARENT DIR>" */

        clear_flag_2d44_thunk();
        draw_string_body(MK_FP(t.seg, t.off),
                         (int16_t)(x + 4), (int16_t)(y + 4));
        restore_cursor_following();

        y    += 0x0a;
        room -= 0x0a;
        i++;
    }
}

/*
 * 0x13a8a
 *
 * **Fill the listing.** Two pointers walk the block `picker_begin` set up: one
 * along the far-pointer array at its front, one along the text after it. Each
 * entry files a pointer and appends its text, and the array is terminated with
 * a null far pointer rather than a count - though a count is kept at 0x5693 as
 * well, because the fill also has to stop when the block is full.
 *
 * **The three record shapes are what `path_join` reads back.** A directory is
 * written `<NAME>`: `<`, then the name copied *including its NUL*, then the
 * byte before the write pointer - which is that NUL - overwritten with `>` and
 * a fresh NUL put down. That is where `path_join`'s off-by-one at both ends
 * comes from, and it is no longer a guess.
 *
 * A file is written as a fixed 8-character stem padded with spaces and then
 * everything from the `.` onwards, so the extensions line up in a column
 * without the drawing code measuring anything.
 *
 * And a lone `:` goes in first when DGROUP 0x53ae is not zero - the fourth byte
 * of the current directory, so "there is something past X:\\". It is the way
 * back up, and it is needed because `.` and `..` are both thrown away below.
 *
 * **The extension filter reads DGROUP when a name has no dot.** `strchr` for
 * `.` answers zero for a name like README, and the three comparisons that
 * follow are made through that zero, against DGROUP's own first bytes. Near
 * pointers make it harmless rather than fatal, and it is why a pattern of
 * `*.*` - whose second byte is `*` - is turned into *no filter at all* before
 * the loop starts, rather than into a filter that always matches.
 */
void sub_13a8a(const volatile uint8_t * pattern)
{
    /* Two cursors into the one block: the array of far pointers at its
       front, and the text they point at. `ptr` walks four bytes at a time
       and `txt` a byte at a time, both inside one segment - so the array is
       a `struct far_ptr *` and the text a plain byte cursor. */
    struct far_ptr far *ptr;            /* [bp-4], [bp-2]: into the array */
    /* **A pair, although it is only ever written through.** Its value is
       *stored* into the array above at each entry, and the original keeps
       the segment fixed while the offset grows - so a host pointer is not
       an option: `FP_SEG`/`FP_OFF` would answer the normalised pair, which
       is different bytes in a block the comparison reads. */
    struct far_ptr txt;                 /* [bp-8], [bp-6]: into the text */
    const volatile uint8_t * want_ext;                  /* [bp+6], rewritten in place */
    volatile uint8_t *  name;                      /* di */
    const volatile uint8_t * name_ext;                  /* [bp-0xa]                        */
    int16_t  n;                         /* [bp-0xe]                        */
    uint16_t more;                      /* [bp-0xc]                        */

    DG568F.entry_count = 0;
    dos_get_cur_dir(dg_off(dgroup, DG530B.path_field));

    ptr = (struct far_ptr far *)MK_FP(DG568F.block.seg, DG568F.block.off);
    txt.seg = (uint16_t)DG568F.word_5697;
    txt.off = (uint16_t)DG568F.entry_size;

    want_ext = string_chr((volatile uint8_t *)pattern, '.');
    if (want_ext != NULL && want_ext[1] == '*')
        want_ext = NULL;

    if (DG53AB.byte_53ae != 0) {
        *ptr++ = txt;

        FAR8(txt.seg, txt.off) = ':';
        txt.off++;
        FAR8(txt.seg, txt.off) = 0;
        txt.off++;

        DG568F.entry_count++;
    }

    more = dos_findfirst(0x2956 /* "*.*" */, 0x10);

    while (more == 0 && ((uint16_t)DG568F.entry_count) < ((uint16_t)DG568F.word_569d)) {
        name     = dos_find_name();
        name_ext = string_chr(name, '.');

        if ((dos_find_attr() & 0x10) != 0) {
            if (string_compare(name, dg_ptr(dgroup, 0x295a /* "." */)) != 0
                && string_compare(name,
                                  dg_ptr(dgroup, 0x295c /* ".." */)) != 0) {
                *ptr++ = txt;
                DG568F.entry_count++;

                FAR8(txt.seg, txt.off) = '<';
                txt.off++;

                do {
                    FAR8(txt.seg, txt.off) = *name;
                    txt.off++;
                } while (*name++ != 0);

                FAR8(txt.seg, (uint16_t)(txt.off - 1)) = '>';
                FAR8(txt.seg, txt.off)                 = 0;
                txt.off++;
            }
        } else if (want_ext == NULL
                   || (name_ext[1] == want_ext[1]
                       && name_ext[2] == want_ext[2]
                       && name_ext[3] == want_ext[3])) {
            *ptr++ = txt;
            DG568F.entry_count++;

            n = 0;
            while (*name != 0 && *name != '.') {
                FAR8(txt.seg, txt.off) = *name;
                name++;
                txt.off++;
                n++;
            }

            while (n < 8) {
                FAR8(txt.seg, txt.off) = ' ';
                txt.off++;
                n++;
            }

            do {
                FAR8(txt.seg, txt.off) = *name;
                txt.off++;
            } while (*name++ != 0);
        }

        more = dos_findnext(0x295f /* "*.*" */, 0x10);
    }

    *ptr = FAR_NULL;                    /* the list's terminator */
}

/*
 * 0x13c78
 *
 * **Sort the listing**, by exchanging the far pointers at the front of the
 * block and never the text they point at. A bubble sort: passes until one makes
 * no exchange.
 *
 * **Directories come first, and the test is the `<` the fill wrote.** There is
 * no type field to consult - the first byte of the record *is* the type - so
 * one against a directory sorts before, one after, and two of a kind fall
 * through to comparing the names. Which is why the four cases are written as
 * two nested tests rather than a comparison of two flags.
 *
 * The names are compared with `far_stricmp`, so `<Dos>` and `<DOS>` land where
 * a reader expects rather than where ASCII puts them.
 *
 * The `:` entry is **not sorted**: every pass starts one slot further in when
 * the first record begins with it, so the way back up stays at the top however
 * the rest moves.
 *
 * The walk ends on a null far pointer, and it checks *two* - the current slot
 * and the next - because a bubble pass compares a pair and there is no pair at
 * the last entry.
 */
void sub_13c78(void)
{
    /* The block is an array of far pointers, one per entry. `p` walks it
       and `q` is always `p + 1`, which is what the original's `+ 4` is. */
    struct far_ptr far *p;              /* [bp-4], [bp-2] */
    int16_t  swapped = 1;

    while (swapped) {
        swapped = 0;

        p = (struct far_ptr far *)MK_FP(DG568F.block.seg,
                                        DG568F.block.off);

        /* Skip the ":" entry - the current directory - if it is first, so
           the sort below never moves it. */
        if (!far_eq(p[0], FAR_NULL)) {
            if (FAR8(p[0].seg, p[0].off) == ':')
                p++;
        }

        while (!far_eq(p[0], FAR_NULL) && !far_eq(p[1], FAR_NULL)) {
            struct far_ptr a = p[0];
            struct far_ptr b = p[1];
            int16_t  swap = 0;

            /* "<PARENT DIR>" and the directories sort first. */
            if (FAR8(a.seg, a.off) != '<' && FAR8(b.seg, b.off) == '<')
                swap = 1;
            else if (FAR8(a.seg, a.off) == '<' && FAR8(b.seg, b.off) != '<')
                swap = 0;
            else if (far_stricmp((const char far *)MK_FP(a.seg, a.off),
                                 (const char far *)MK_FP(b.seg, b.off)) > 0)
                swap = 1;

            if (swap) {
                p[0] = b;
                p[1] = a;
                swapped = 1;
            }

            p++;
        }
    }
}

/*
 * 0x13606
 *
 * **Get the listing a place to live, then fill it and draw it.**
 *
 * The block is allocated once and kept: the far pointer at DGROUP 0x5699 being
 * non-null is the whole test, and on the second and later openings everything
 * below is skipped. There is a second source before DOS is asked at all - the
 * pointer at 0x3576, some other part of the game's block, which is taken with a
 * flat capacity of 0x3e8 entries rather than a measured one.
 *
 * Otherwise it asks `dos_alloc_bytes` for **0xffffffff** bytes, which is the
 * "how much is there" question, and clamps the answer to 0x7530. So the picker
 * takes what is free up to 30000 bytes and no more - the listing is allowed to
 * grow into spare memory, but not to eat it.
 *
 * The entry size is 0x16, and that is where 0x5695 comes from: the block is
 * *two* arrays, `count` far pointers of four bytes each and then the records
 * themselves, so the second pointer is the first plus `4 * count`. Only the
 * offset is added - the segment is shared - which is what keeps a listing this
 * size inside one segment.
 */
void picker_begin(uint16_t arg1, uint16_t arg2, const volatile uint8_t * pattern)
{
    uint32_t v;

    (void)arg1;
    (void)arg2;

    if ((DG568F.block.off | DG568F.block.seg) == 0) {
        if ((DG3576.scratch.off | DG3576.scratch.seg) != 0) {
            DG568F.word_569d = 0x3e8;
            DG568F.block.seg = DG3576.scratch.seg;
            DG568F.block.off = DG3576.scratch.off;
        } else {
            v = dos_alloc_bytes(0xffffffffu, 0, 0).bytes;

            if ((int32_t)v > 0x7530)
                v = 0x7530;

            DG568F.word_569d = (uint16_t)long_divide((int32_t)v, 0x16);

            DG568F.block = dos_alloc_bytes(v, 0, 0).ptr;
        }

        DG568F.word_5697 = DG568F.block.seg;
        DG568F.entry_size = (uint16_t)(DG568F.block.off + 4 * ((uint16_t)DG568F.word_569d));
    }

    sub_13a8a(pattern);
    sub_13c78();
    DG568F.scroll = 0;
}

/*
 * 0x13870
 *
 * **The name field.** The buffer at DGROUP 0x53ab is copied into a local first,
 * and then the *pointer* is walked forward while the text is wider than 0xac
 * pixels - so a long name scrolls off the **left**, showing its end. That is
 * the right way round for typing: what you just typed stays in view.
 *
 * The caret is `*`, and it blinks by counting: 0x567e is bumped on every one of
 * these redraws and bit 3 decides whether the asterisk is appended, so it is on
 * for eight redraws and off for eight. It is appended *after* the width walk,
 * which means the caret can push the text past 0xac - the field is measured on
 * the name, not on the name plus caret.
 *
 * It only blinks when 0x4e6b says 0x4000, the field's own mode. Out of that
 * mode nothing is counted, so the caret is not merely hidden, it stops.
 */
void picker_draw_name(void)
{
    uint8_t buf[90];                  /* [bp-0x5a] */
    uint8_t *si  = buf;

    string_copy((volatile uint8_t *)buf, (volatile uint8_t *)DG530B.path_field);

    while ((int16_t)text_width_thunk(si) > 0xac)
        si++;

    if (DG4E67.state == 0x4000) {
        DG5677.caret_blink++;
        if ((DG5677.caret_blink & 8) != 0)
            string_concat(si, dg_ptr(dgroup, 0x2952 /* "*" */));
    }

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    fill_panel_area(0x40, 0x56, 0xb8, 0x10, 0);

    DG3890.unknown_01 = 0;
    DG3890.unknown_00 = 0x0f;

    clear_flag_2d44_thunk();
    draw_string(si, 0x44, 0x5a);
    restore_cursor_following();
}

/*
 * 0x136c9
 *
 * **The picker's whole screen.** Everything the loop redraws piecemeal, laid
 * down once: the title bar, the four sunken wells - two for the buttons, two
 * for the scroll arrows - the heading, the buttons, and then the same four
 * routines the loop calls for its partial repaints.
 *
 * The heading and the left button say LOAD or SAVE according to 0x568f, and
 * they are drawn from **two separate branches** rather than one branch choosing
 * two strings. Both buttons are drawn unpressed here; `picker_draw_action` is
 * what draws them pressed, and it exists precisely because this routine cannot
 * be called for a button going down.
 *
 * The wells are placed round what goes in them, not derived from it: the button
 * well at (0x36, 0x129) is 0x40 by 0x20 for a button drawn at (0x40, 0x130).
 */
void picker_repaint(void)
{
    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    draw_title_bar(0x30, 0x31, 0x110, 0x149, 1);

    draw_sunken_box(0x36, 0x129, 0x40, 0x20);
    draw_sunken_box(0xb6, 0x129, 0x50, 0x20);

    if (((uint16_t)DG568F.picker_mode) == 0x100) {
        draw_scroll_text(dg_ptr(dgroup, 0x219e /* "LOAD MACHINE" */), 0x50, 0x34, 0xa0);
        draw_button(0x21b8 /* "LOAD" */, 0x40, 0x130, 0);
    } else {
        draw_scroll_text(dg_ptr(dgroup, 0x21ab /* "SAVE MACHINE" */), 0x50, 0x34, 0xa0);
        draw_button(0x21bd /* "SAVE" */, 0x40, 0x130, 0);
    }

    draw_sunken_box(0xbc, 0x74, 0x20, 0x20);
    draw_sunken_box(0xbc, 0xe0, 0x20, 0x20);

    picker_draw_up();
    picker_draw_down();

    draw_button(0x21c2 /* "CANCEL" */, 0xc0, 0x130, 0);

    picker_draw_name();
    picker_draw_list();
    picker_draw_filename();

    present_back_page();
}

/*
 * 0x137e4
 *
 * **The list's up arrow**, redrawn. Which of the two pieces of art it uses is
 * read out of the mode word DGROUP 0x4e6b - 0x800 is "this arrow is held down"
 * - so the picker never has to tell it, the same way `picker_draw_action` reads
 * its own word back.
 *
 * The pair sits at +0x4a in the art set at DGROUP 0x52f4, and the pressed one
 * is the *second*, which is why the index is doubled before it is added.
 */
void picker_draw_up(void)
{
    int16_t pressed = (DG4E67.state == 0x800) ? 1 : 0;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[pressed + 0x25]),
                0xc4, 0x78, 0);
    restore_cursor_following();
}

/*
 * 0x1382a
 *
 * **The list's down arrow.** `picker_draw_up`'s twin, and the only differences
 * are the three numbers: the mode it answers to is 0x400, its art is at +0x4e,
 * and it sits 0x70 further down at y 0xe8.
 */
void picker_draw_down(void)
{
    int16_t pressed = (DG4E67.state == 0x400) ? 1 : 0;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(BMPSET(DG52ED.panel_art_ptr).bmp[pressed + 0x27]),
                0xc4, 0xe8, 0);
    restore_cursor_following();
}

/*
 * NOT a transcription: the port's factoring of the eleven **identical inline
 * blocks** at 0x13205 to 0x133c3. Each is `strnicmp` against one reserved DOS
 * device name followed by a check that the byte after it ends the stem, and the
 * original repeats the whole thing eleven times rather than looping. Every
 * constant is kept, in the order the original tests them - including the last
 * pair, which do not agree with each other.
 */
static const struct {
    uint16_t name;
    uint16_t len;
    uint16_t after;
} reserved_names[] = {
    { 0x291c, 3, 3 },   /* "con"  */
    { 0x2920, 3, 3 },   /* "aux"  */
    { 0x2924, 4, 4 },   /* "com1" */
    { 0x2929, 4, 4 },   /* "com2" */
    { 0x292e, 4, 4 },   /* "com3" */
    { 0x2933, 4, 4 },   /* "com4" */
    { 0x2938, 3, 3 },   /* "prn"  */
    { 0x293c, 4, 4 },   /* "lpt1" */
    { 0x2941, 4, 4 },   /* "lpt2" */
    { 0x2946, 3, 3 },   /* "nul"  */
    { 0x294a, 3, 4 },   /* "null" compared for THREE bytes - see below */
};

/*
 * 0x1319d
 *
 * **Is the typed name usable?** Three answers, not two: 0 for no, 1 for a name
 * that is free to create, and **2 for one that already exists** - which the
 * caller needs to tell apart so it can ask before overwriting.
 *
 * The rejections, in the order they are made:
 *
 *   an empty name, or one starting with `.`; a space anywhere in the stem - the
 *   scan stops at the first `.`, so spaces in an extension are not looked at;
 *   any of the fourteen characters in the table at DGROUP 0x28ec, which are
 *   `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`; and any of eleven
 *   reserved DOS device names.
 *
 * A device name only counts when it is the *whole stem* - the byte after it has
 * to be the terminator or a `.` - which is why `CONFIG.TIM` survives and
 * `CON.TIM` does not.
 *
 * **The eleventh entry is wrong in the original.** "null" is compared for
 * **three** bytes - so it tests the same `nul` the tenth entry does - but checks
 * the byte at +4 rather than +3. The effect is that *any* four-letter stem
 * beginning `NUL` is rejected: `NULA` and `NULX` as much as `NULL`. Written as
 * `strnicmp(name, "null", 4)`, which is plainly what was meant, it would have
 * caught `NULL` alone. Transcribed as it is.
 *
 * Last, it *opens the file* to find out whether it is there, and closes it
 * again. A name that opens answers 2. One that does not answers 1 only when
 * 0x568f says 0x80 - the mode the picker was opened from - and 0 otherwise, so
 * asking to load something that is not there is a rejection rather than an
 * answer the caller has to interpret.
 */
uint16_t validate_filename(void)
{
    uint16_t si, i, file;
    int16_t  bad = 0;

    si = dg_off(dgroup, DG4E4E.name_buf);

    if (DG8(si) == 0)
        bad = 1;
    if (DG8(si) == '.')
        bad = 1;

    while (DG8(si) != 0 && DG8(si) != '.') {
        if (DG8(si) == ' ')
            bad = 1;
        si++;
    }

    if (bad)
        return 0;

    for (i = 0; i < 0x0e; i++) {
        if (string_chr((volatile uint8_t *)DG4E4E.name_buf,
                       DG8((uint16_t)(0x28ec + i))) != NULL)
            return 0;
    }

    for (i = 0; i < sizeof reserved_names / sizeof reserved_names[0]; i++) {
        uint16_t after;

        if (string_ncompare_i(dg_off(dgroup, DG4E4E.name_buf), reserved_names[i].name,
                              reserved_names[i].len) != 0)
            continue;

        after = (uint8_t)DG4E4E.name_buf[reserved_names[i].after];
        if (after == 0 || after == '.')
            return 0;
    }

    file = game_fopen((volatile uint8_t *)DG4E4E.name_buf, dg_ptr(dgroup, 0x294f /* "rb" */));

    if (file != 0) {
        game_fclose(file);
        return 2;
    }

    if (((uint16_t)DG568F.picker_mode) == 0x80)
        return 1;

    return 0;
}

/*
 * 0x13402
 *
 * **Redraw the picker's one action button.** Which word it carries is not a
 * parameter: it is read back out of the mode word DGROUP 0x4e6b, and when that
 * says 0x200 - the picker's own mode - out of *0x568f*, the value 0x4e6b held
 * before the picker took it. So the button says LOAD or SAVE according to which
 * handler opened the picker, and the picker itself does not have to be told.
 *
 * Anything else says CANCEL, and it moves: 0xc0 against 0x40. The two are
 * different buttons in the same place in the code, not one button relabelled.
 */
void picker_draw_action(void)
{
    if (DG4E67.state != 0x200) {
        draw_button(0x21c2 /* "CANCEL" */, 0xc0, 0x130, 1);
    } else if (((uint16_t)DG568F.picker_mode) == 0x100) {
        draw_button(0x21b8 /* "LOAD" */, 0x40, 0x130, 1);
    } else {
        draw_button(0x21bd /* "SAVE" */, 0x40, 0x130, 1);
    }

    present_back_page();
}

/*
 * 0x13902
 *
 * **The "File Name:" field**, and `picker_draw_name`'s twin down to the shape
 * of the code: copy, walk the pointer forward while the text is too wide, blink
 * a caret by counting, fill, draw.
 *
 * Everything that differs is a number - a different buffer (0x4e5a against
 * 0x53ab), a narrower field (0x64 against 0xac), a different mode (0x1000), a
 * different counter (0x5680) - and a *different asterisk*: 0x2954, where the
 * other field uses 0x2952. The two one-character strings sit next to each other
 * in the image, unpooled, which is how you can tell these are two routines and
 * not one called twice.
 *
 * This one also draws its own label, because the label belongs to the field.
 */
void picker_draw_filename(void)
{
    uint8_t buf[16];                  /* [bp-0x10] */
    uint8_t *si  = buf;

    string_copy((volatile uint8_t *)buf, (const volatile uint8_t *)DG4E4E.name_buf);

    while ((int16_t)text_width_thunk(si) > 0x64)
        si++;

    if (DG4E67.state == 0x1000) {
        DG5677.caret_blink_b++;
        if ((DG5677.caret_blink_b & 8) != 0)
            string_concat(si, dg_ptr(dgroup, 0x2954 /* "*" */));
    }

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    draw_scroll_text(dg_ptr(dgroup, 0x21c9 /* "File Name:" */), 0x30, 0x10c, 0x54);
    fill_panel_area(0x90, 0x10c, 0x70, 0x10, 0);

    DG3890.unknown_01 = 0;
    DG3890.unknown_00 = 0x0f;

    clear_flag_2d44_thunk();
    draw_string(si, 0x94, 0x110);
    restore_cursor_following();
}

/*
 * 0x1345f
 *
 * **Tab inside the picker**, and the same trick as the panel's at 0x1156c: it
 * warps the pointer rather than moving any focus. Seven stops, cursor at DGROUP
 * 0x28fa, x at 0x28fc and y at 0x290a - and here the two tables are the same
 * length, because none of the picker's controls is a slider whose position has
 * to be worked out from a value.
 */
void picker_tab(void)
{
    DG28FA.word_28fa++;

    if (DG28FA.word_28fa == 7)
        DG28FA.word_28fa = 0;

    move_pointer_to(DG16((uint16_t)(0x28fc + 2 * DG28FA.word_28fa)),
                    DG16((uint16_t)(0x290a + 2 * DG28FA.word_28fa)));
}

/*
 * 0x13490
 *
 * **One keystroke into the picker's name field.** Backspace - 8 - takes the
 * last byte off, and does nothing on an empty field. Anything else is appended
 * *as a string*: the character is stored into a two-byte local with a NUL after
 * it and handed to `strcat`, which is why this routine has locals at all.
 *
 * Two characters never reach the field: backspace, which is handled above, and
 * **tab**, which is excluded explicitly. Tab is a key the picker wants for
 * moving the pointer, and a field that swallowed it would take it away.
 *
 * The length check is `< max`, and `max` counts the NUL's room the way the
 * caller passed it - this routine does not add one.
 */
void picker_type(uint8_t c, uint16_t buf, int16_t max)
{
    uint8_t str[2];                  /* [bp-2], the two-byte string */
    int16_t  len;

    (*str)                       = c;
    str[1]                     = 0;

    len = (int16_t)string_length(dg_ptr(dgroup, buf));

    if (c == 8) {
        if (len != 0)
            DG8((uint16_t)(buf + len - 1)) = 0;
    } else if (len < max && c != 9) {
        string_concat(dg_ptr(dgroup, buf), (volatile uint8_t *)str);
    }
}

/*
 * 0x134dd
 *
 * **Is this path a drive's root?** One separator in the whole string, and it is
 * the last byte - "C:\\" and nothing else. It counts the same way `path_up`
 * does, against the same shared "\\" at DGROUP 0x1bca, which is what keeps the
 * two agreeing about where the walk up has to stop.
 */
uint16_t path_is_root(uint16_t path)
{
    uint16_t si = path;
    uint16_t last = 0;
    int16_t  n = 0;

    while (DG8(si) != 0) {
        if (DG8(si) == DG8(DG1BCA.word_1bca)) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1 && DG8((uint16_t)(last + 1)) == 0)
        return 1;

    return 0;
}

/*
 * 0x13516
 *
 * **Drop the last component of a path**, in place. It walks to the terminator
 * counting separators - the character is not a literal here but `*DG1BCA.word_1bca`,
 * the one-character string "\\" the rest of the module shares - and remembers
 * the last one it saw.
 *
 * The two cases differ by one byte, and that byte is the whole point: with a
 * single separator the cut is *after* it, leaving "C:\\", because a drive with
 * its backslash taken off means the current directory rather than the root.
 * With more than one it cuts *at* the separator, leaving the parent. With none
 * it does nothing at all.
 */
void path_up(uint16_t path)
{
    uint16_t si = path;
    uint16_t last = 0;
    int16_t  n = 0;

    while (DG8(si) != 0) {
        if (DG8(si) == DG8(DG1BCA.word_1bca)) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1)
        DG8((uint16_t)(last + 1)) = 0;
    else if (n > 1)
        DG8(last) = 0;
}

/*
 * 0x1354c
 *
 * **Join a listed name onto the path.** The name comes in as a *far* pointer -
 * it is in the picker's own list block, not DGROUP - and the path is near, so
 * the name is copied through a fourteen-byte local first.
 *
 * That copy is off by one at both ends, deliberately, and `sub_13a8a` is what
 * makes it right: a directory is written into the listing as `<NAME>`. This
 * stores from the **second** byte, past the `<`, and after the join chops the
 * **last** byte, the `>`. So `<DOS>` arrives and `\\DOS` leaves.
 *
 * The separator goes in only when the path is not already a root, because a
 * root already ends in one and `path_is_root` is the routine that knows.
 *
 * The loop tests the byte *before* stepping and stores the byte *after*, so the
 * NUL is copied along with the rest and the local needs no terminating of its
 * own.
 */
void path_join(uint16_t path, const char far * entry)
{
    uint8_t name[14];                            /* [bp-0xe] */
    uint16_t di   = 0;
    uint16_t len;

    while (*entry != 0) {
        entry++;
        name[di] = *entry;
        di++;
    }

    if (path_is_root(path) == 0)
        string_concat(dg_ptr(dgroup, path), dg_ptr(dgroup, DG1BCA.word_1bca));

    string_concat(dg_ptr(dgroup, path), name);

    len = string_length(dg_ptr(dgroup, path));
    DG8((uint16_t)(path + len - 1)) = 0;
}

/*
 * 0x135a6
 *
 * **Force a name into 8.3.** The eighth byte is cut off first, unconditionally
 * and before anything is looked at, so a long name loses its tail rather than
 * its extension. Then the first `.` - or the terminator, if there is none - is
 * where the new one goes, and the extension the caller passed is appended.
 *
 * An empty name is left empty: the cut at byte 8 has already happened, but
 * nothing is appended, so the picker cannot end up holding a name that is only
 * an extension.
 */
void force_extension(uint16_t name, uint16_t ext)
{
    uint16_t si;

    DG8((uint16_t)(name + 8)) = 0;

    if (DG8(name) == 0)
        return;

    si = name;
    while (DG8(si) != 0 && DG8(si) != '.')
        si++;

    DG8(si)                    = '.';
    DG8((uint16_t)(si + 1))    = 0;

    string_concat(dg_ptr(dgroup, name), dg_ptr(dgroup, ext));
}

/*
 * 0x135dc
 *
 * Hand the picker a name to start from: a straight copy into DGROUP 0x4e5a,
 * the one buffer the picker answers out of.
 *
 * **Nothing calls it**, by three searches rather than one: no near `call` to
 * it in this module, no far `call` anywhere in the image - `9a ec 55 ff 0d` -
 * and its far pointer `0dff:55ec` is not *stored* anywhere either, which is how
 * the region handlers are reached and would have been missed by the first two.
 * The same holds for `picker_name` beside it.
 *
 * The pointer search was run against `path_join` as a control: that one is
 * called, by a near `call`, and its pointer is likewise stored nowhere - so the
 * search distinguishes dispatch through a table from a direct call, rather than
 * answering "nowhere" to everything. They are the picker's public face, written and never
 * used, because `pick_file` fills 0x4e5a itself and copies the answer to
 * 0x52fe on the way out. Transcribed because they are there, and recorded as
 * dead because saying "unreached on the paths tried" would suggest a path
 * exists.
 */
void picker_set_name(uint16_t name)
{
    string_copy((volatile uint8_t *)DG4E4E.name_buf, dg_ptr(dgroup, name));
}

/*
 * 0x135ef
 *
 * The picker's answer: **the buffer's address, or zero when it is empty.** A
 * caller would get a pointer it could hand straight to `load_animation`,
 * without having to know where the name lives.
 *
 * There is no caller. See `picker_set_name` above: neither is reachable from
 * anywhere in the image.
 */
uint16_t picker_name(void)
{
    if (DG4E4E.name_buf[0] != 0)
        return dg_off(dgroup, DG4E4E.name_buf);

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
void write_byte(uint16_t file, const volatile uint8_t * addr)
{
    if (DG546C.error != 0)
        return;

    if (game_fwrite(addr, 1, 1, file) != 1)
        DG546C.error = 1;
}

/*
 * 0x123e4
 *
 * **Write one word.** The same routine as `write_byte` with a size of 2, and
 * the original writes it out twice rather than sharing one - so this does too.
 */
void write_word(uint16_t file, const volatile uint8_t * addr)
{
    if (DG546C.error != 0)
        return;

    if (game_fwrite(addr, 2, 1, file) != 1)
        DG546C.error = 1;
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
        write_byte(file, dg_ptr(dgroup, str));
        if (DG8(str) == 0)
            return;
        str++;
    }
}

/*
 * 0x11d00
 *
 * **A part's index among all parts**, which is how the machine file refers to
 * one: a pointer means nothing to a reload, so every reference is written as the
 * position the part has in the walk `pick_by_flag(0x3000)` makes.
 *
 * A null part answers 0xffff, and that is the file's "no part here".
 *
 * **A part that is not found answers the count**, because the loop ends the same
 * way whether it found the part - which sets `si` to zero to break out - or ran
 * off the end, and the index is whatever the counter reached. So a reference to
 * something outside the walk is written as one past the last part rather than
 * as an error. Nothing here checks for it, and this is transcribed as it is
 * rather than made to answer 0xffff, because a reload that trips over it is
 * behaviour the original has.
 */
uint16_t part_index(uint16_t part)
{
    uint16_t si;
    uint16_t n = 0;

    if (part == 0)
        return 0xffff;

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0; ) {
        if (si == part) {
            si = 0;
            break;
        }
        si = (uint16_t)pick_for_record(si, 0x1000);
        n++;
    }

    return n;
}

/*
 * 0x12430
 *
 * **Write one part's record.** Thirteen fields, then whatever the part is
 * attached to - and every attachment is written as a *`part_index`*, never a
 * pointer, so a reload can find the other end again.
 *
 * **Three of its locals have their addresses taken**, because `write_word`
 * writes from an address and the values here are computed rather than fields of
 * the part: whether there is a rope, whether there is a belt, and each index in
 * turn. So the port takes a guest frame for those three and keeps the rest as
 * ordinary locals - which is the same split `sub_126ec` needed for its count.
 *
 * **The rope flag is written whether or not there is a rope**, and the belt flag
 * twice, once per slot. That is what makes the record fixed-width up to the
 * flags and self-describing after them: a reader takes the flag and knows
 * whether two more indices follow.
 *
 * The belt flag can only be true on the **first** slot - `i == 0` and the kind
 * being 0x0a or 7 - which is why the belt it then reads is at +0x66 flatly and
 * not at +0x66 + 2i. The second pass writes the flag as zero and the two bytes
 * at +0x6a and +0x6b, and nothing else.
 *
 * Then two runs over the link array: slots 0 and 1, then slots **4 and 5** -
 * skipping 2 and 3, which are the second half of the pairs `detach_belt` and
 * `sub_05482` clear together. A file that stored them would be storing the same
 * links twice.
 *
 * Last, and only for kind 7, the record at +0x68 - its first word as an index,
 * or 0xffff when there is none. That is the one place this writes 0xffff
 * itself; everywhere else it comes back from `part_index`.
 */
void sub_12430(uint16_t file, struct part *part)
{
    int16_t vindex;   /* [bp-6] */
    int16_t vbelt;   /* [bp-4] */
    int16_t vrope;/* [bp-2] */
    uint16_t rope, belt;
    int16_t  i;

    write_word(file, (const volatile uint8_t *)&part->kind);
    write_word(file, (const volatile uint8_t *)&part->flags_06);
    write_word(file, (const volatile uint8_t *)&part->word_94);
    write_word(file, (const volatile uint8_t *)&part->flags_0a);
    write_word(file, (const volatile uint8_t *)&part->word_90);
    write_word(file, (const volatile uint8_t *)&part->word_92);
    write_word(file, (const volatile uint8_t *)&part->width);
    write_word(file, (const volatile uint8_t *)&part->height);
    write_word(file, (const volatile uint8_t *)&part->word_50);
    write_word(file, (const volatile uint8_t *)&part->word_52);
    write_word(file, (const volatile uint8_t *)&part->word_8c);
    write_word(file, (const volatile uint8_t *)&part->word_8e);
    write_word(file, (const volatile uint8_t *)&part->word_96);

    vrope = (int16_t)(uint16_t)(((int16_t)part->kind) == 8 ? 1 : 0);
    write_word(file, (volatile uint8_t *)&vrope);

    write_byte(file, (const volatile uint8_t *)&part->grab_x);
    write_byte(file, (const volatile uint8_t *)&part->grab_y);
    write_word(file, (const volatile uint8_t *)&part->word_58);

    if ((uint16_t)vrope != 0) {
        rope = part->word_54;

        vindex = (int16_t)part_index(ROPE(rope).end_a_ptr);
        write_word(file, (volatile uint8_t *)&vindex);
        vindex = (int16_t)part_index(ROPE(rope).end_b_ptr);
        write_word(file, (volatile uint8_t *)&vindex);
    }

    for (i = 0; i < 2; i++) {
        vbelt = (int16_t)(uint16_t)((i == 0
                                   && (((int16_t)part->kind) == 0x0a
                                       || ((int16_t)part->kind) == 7))
                                  ? 1 : 0);
        write_word(file, (volatile uint8_t *)&vbelt);

        write_byte(file, &part->attach[i].x);
        write_byte(file, &part->attach[i].y);

        if ((uint16_t)vbelt != 0) {
            belt = part->word_66;

            vindex = (int16_t)part_index(BELT(belt).end_a_ptr);
            write_word(file, (volatile uint8_t *)&vindex);
            vindex = (int16_t)part_index(BELT(belt).end_b_ptr);
            write_word(file, (volatile uint8_t *)&vindex);

            write_byte(file, dg_ptr(dgroup, (uint16_t)(belt + 0x0a)));
            write_byte(file, dg_ptr(dgroup, (uint16_t)(belt + 0x0b)));
        }
    }

    for (i = 0; i < 2; i++) {
        vindex = (int16_t)part_index(part->link[i]);
        write_word(file, (volatile uint8_t *)&vindex);
    }

    for (i = 4; i < 6; i++) {
        vindex = (int16_t)part_index(part->link[i]);
        write_word(file, (volatile uint8_t *)&vindex);
    }

    if (((int16_t)part->kind) == 7) {
        belt = part->word_68;

        if (belt != 0)
            vindex = (int16_t)part_index(BELT(belt).owner_ptr);
        else
            vindex = (int16_t)0xffff;

        write_word(file, (volatile uint8_t *)&vindex);
    }
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
        else if (DG546C.is_level != 0)
            DGU16((uint16_t)(p + 6)) |= 0x8000;

        sub_12430(file, PARTP(p));
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
    int16_t vn;                   /* [bp-2] */
    uint16_t p;

    vn = (int16_t)0;
    for (p = DGU16(head); p != 0; p = DGU16(p))
        vn++;

    write_word(file, (volatile uint8_t *)&vn);
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

    DG546C.error = 0;
    DG546C.version_out = 0xaced;
    DG546C.version = 0x0102;
    DG4E67.file_op_active = 1;

    f = game_fopen(dg_ptr(dgroup, name), dg_ptr(dgroup, 0x2873));       /* "wb" */
    if (f == 0) {
        DG4E67.file_op_active = 0;
        return 1;
    }

    write_word(f, dg_ptr(dgroup, 0x5476));
    write_word(f, dg_ptr(dgroup, 0x5474));

    if (DG546C.is_level != 0) {
        write_string(f, 0x4ecf);
        write_string(f, 0x4f1f);
        write_word(f, dg_ptr(dgroup, 0x50af));
        write_word(f, dg_ptr(dgroup, 0x50b1));
    }

    write_word(f, dg_ptr(dgroup, 0x50b3));
    write_word(f, dg_ptr(dgroup, 0x50b5));

    if (DG546C.is_level != 0) {
        write_word(f, dg_ptr(dgroup, 0x50b7));
        write_word(f, dg_ptr(dgroup, 0x50b9));
    }

    write_word(f, dg_ptr(dgroup, 0x50bb));

    sub_126ec(f, 0x521b);
    sub_126ec(f, 0x5179);
    sub_126ec(f, 0x50d7);

    sub_126b3(f, 0x521b, 0);
    sub_126b3(f, 0x5179, 1);
    sub_126b3(f, 0x50d7, 2);

    if (game_fclose(f) != 0)
        DG546C.error = 1;

    if (DG546C.error != 0)
        dos_unlink(name);

    DG4E67.file_op_active = 0;
    return DG546C.error;
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
    uint16_t held = DG50D3.bin_head_ptr;
    uint16_t r;

    DG50D3.bin_head_ptr = 0;
    DG546C.is_level = 0;

    r = sub_1271c(name);

    DG50D3.bin_head_ptr = held;
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
    DG546C.is_level = 0;

    return read_level(dg_ptr(dgroup, name));
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
 * The name is assembled in a stack buffer whose address is passed on. That
 * used to mean a real DGROUP frame; it stopped meaning it when `game_fopen`
 * and the string routines took pointers, and the buffer is a C array.
 */
void count_level_files(void)
{
    uint8_t name[16];                         /* [bp-0x18] */
    uint8_t number[8];    /* [bp-8]    */
    int16_t done = 0;

    DG4E67.level_count = 1;

    while (done == 0) {
        uint16_t file;

        string_copy((volatile uint8_t *)name, dg_ptr(dgroup, 0x2887));              /* "l"    */
        int_to_string((int16_t)((uint16_t)DG4E67.level_count),
                      (volatile uint8_t *)number, 10);
        string_concat((volatile uint8_t *)name, (volatile uint8_t *)number);
        string_concat((volatile uint8_t *)name, dg_ptr(dgroup, 0x2889));            /* ".lev" */

        file = game_fopen((volatile uint8_t *)name, dg_ptr(dgroup, 0x288e));        /* "rb"   */

        if (file != 0) {
            DG4E67.level_count++;
            game_fclose(file);
        } else {
            DG4E67.level_count--;
            done = 1;
        }
    }
}

/*
 * 0x12b60
 *
 * Read the `count`th line of **password.txt** into `buf`.
 *
 * The file has one password a line and this wants the one for a level, so it
 * reads `count` lines and keeps only the last - the buffer is written over
 * each time round. There is no seek and no index; the lines are found by
 * reading past them.
 *
 * `buf` is emptied first, so a missing file leaves an empty string rather than
 * whatever was there: the open is tested and everything else skipped.
 *
 * The loop decrements *before* it reads, and its test is at the top, so a
 * count of zero reads nothing at all and any other count reads exactly that
 * many lines.
 */
void read_password_line(int16_t count, volatile uint8_t * buf)
{
    uint16_t f;

    *buf = 0;

    f = game_fopen(dg_ptr(dgroup, 0x28ab), dg_ptr(dgroup, 0x28b8));         /* "password.txt" */
    if (f == 0)
        return;

    while (count != 0) {
        count--;
        game_fread_line(f, buf);
    }

    game_fclose(f);
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
    uint16_t file = game_fopen(dg_ptr(dgroup, 0x28bb), dg_ptr(dgroup, 0x28c3));

    if (file == 0)
        return 0;

    game_fread_far(file, dg_ptr(dgroup, 0x4eb7));
    game_fread_far(file, dg_ptr(dgroup, 0x4ec1));
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
    free_part_list(PARTP(DG50D3.bin_head_ptr));
    free_part_list(PARTP(DG521B.parts_ptr));
    free_part_list(PARTP(DG5179.moving_ptr));

    DG50D3.bin_head_ptr = 0;
    DG5179.moving_ptr = 0;
    DG521B.parts_ptr = 0;
}

/*
 * 0x14d71
 *
 * Free every part on one list. The next pointer is taken out of the record
 * *before* the record is freed, which is the only way to walk a list you are
 * destroying.
 */
void free_part_list(struct part *p)
{
    while (p != 0) {
        uint16_t next = p->link_ptr;

        free_part(p);
        /* the chain ends on offset 0, and `PARTP(0)` is not null */
        p = next != 0 ? PARTP(next) : 0;
    }
}

