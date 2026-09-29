/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The program**: `main`'s body, the bring-up and the teardown that prints
 * your password.
 *
 * The first module of the original's **code segment 0dff**, image
 * 0x0dfff..0x0e4be, and the first object linked after the C startup: its data
 * is DGROUP 0x00aa..0x0116, straight after C0M's. Functions are in address
 * order and each carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d
 * JUDGE: data 0x00aa..0x0116
 */
#include <stdlib.h>
#include <string.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0dfff
 *
 * **`main`.** The Borland startup calls it at image 0x00155 with argc, argv
 * and envp, and pushes the answer straight into `exit`. The game reads none of
 * the three, which is what a DOS game with no command line looks like.
 *
 * Four calls and nothing else: bring the machine up, the intro, the game, and
 * the teardown, which exits. There is no `return` - no jump to the epilogue
 * after the last call - so the exit status would be whatever AX held, and
 * `game_teardown(1)` never comes back to give one.
 */
void game_main(void)
{
    game_startup();
    game_intro();
    game_play();
    game_teardown(1);
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
    int8_t cfg_byte;                   /* [bp-1] */
    int16_t vm_ok;                     /* [bp-4] */
    int16_t cfg_first;                 /* [bp-6] */
    int16_t sound_device;              /* [bp-8] */
    int16_t sound_module;              /* [bp-0xa] */
    int32_t free_bytes;                /* [bp-0xe] */
    struct queue_node *node;           /* [bp-0x10] */
    struct shape far *block;           /* [bp-0x14] */
    FILE *file;             /* di */
    int16_t i;                         /* si */

    _stklen = 0x800;

    free_bytes = DOS_ALLOC_BYTES(dos_alloc_bytes(0xffffffffUL, 0));
    if (free_bytes < 0x44d90L) {
        printf(g_messages.not_enough_free_memory);
        printf(g_messages.you_need_at_least);
        exit(0);
    }

    dos_get_cur_dir((char *)g_game_directories.game_dir);
    dos_get_cur_dir((char *)g_game_directories.picker_dir);
    set_holiday_flags();

    g_stop_requested = 0;
    g_file_op_active = 0;
    g_cursor = 0xffff;

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

    file = fopen("RESOURCE.CFG", "rb");
    if (file != NULL) {
        fread((uint8_t *)&cfg_byte, 1, 1, file);
        cfg_first = cfg_byte;          /* stored and never read back */
#ifndef __TURBOC__
        (void)cfg_first;
#endif
        fread((uint8_t *)&cfg_byte, 1, 1, file);
        sound_device = cfg_byte;
        fread((uint8_t *)&cfg_byte, 1, 1, file);
        sound_module = cfg_byte;
        fclose(file);
    }

    if (read_tim_cfg() == 0) {
        g_furthest_level = 1;
        g_master_level = 6;
    }

    g_password_puzzle = 0;
    g_banked_score = 0;
    g_fill_colour = 3;
    g_bin_colour = 0x0b;

    vm_ok = vm_init(0x0d, 0x80, (FILE *)WRITABLE_LITERAL("vm.ovl"));
    if (vm_ok == 0) {
        printf(g_messages.unable_to_initialize_vm);
        exit(0);
    }

    g_vmds.page_front = 0xa000;
    g_vmds.page_back = 0xa820;
    vm_set_display_lines(0x1d6);                /* 470 - the Sierra logo */

    g_pal_tim = load_palette(WRITABLE_LITERAL("tim.pal"));
    g_pal_sierra = load_palette(WRITABLE_LITERAL("sierra.pal"));
    set_palette_pointer(g_pal_black = load_palette(WRITABLE_LITERAL("black.pal")));

    set_font(g_memo_font = load_font(WRITABLE_LITERAL("memofnt8.fnt")));

    g_cursor_art = load_bitmap_list(WRITABLE_LITERAL("mouse.bmp"));
    g_panel_art = load_bitmaps(WRITABLE_LITERAL("cp.bmp"));
    g_border_art = load_bitmaps(WRITABLE_LITERAL("gp_bord.bmp"));

    install_keyboard(0);

    start_sound(sound_device, sound_module, 0, (FILE *)WRITABLE_LITERAL("sx.ovl"));

    g_tim_sx = open_file_record(WRITABLE_LITERAL("tim.sx"));
    for (i = 1; i <= 0x14; i++)
        open_sound_file((char *)g_tim_sx, i);

    set_master_level_ok(g_master_level_ok[g_master_level]);

    install_divide_trap();
    timer_install(0x0d);
    mouse_init();
    mouse_move_to(10, 10);
    timer_add_callback(timer_callback, 4);

    select_cursor(0);
    erase_both_pages();
    mouse_set_speed(3);
    build_screen_regions();
    count_level_files();

    /*
     * Twenty eight-byte records off the near heap, chained through their first
     * word. 0x4e56 is the head; 0x4e58 is cleared with it and left alone.
     */
    g_parts_free = g_parts_queue = 0;
    for (i = 0; i < 0x14; i++) {
        node = (struct queue_node *)calloc_far(1, sizeof(struct queue_node));
        node->next = g_parts_free;
        g_parts_free = node;
    }

    /*
     * And 180 twenty-four-byte records from DOS, chained the same way but
     * through a far pointer in the first four bytes of each block. 0x4e52 is
     * the second head, cleared here and not filled.
     */
    g_shape_free = g_shapes_drawn = NULL;
    for (i = 0; i < 0xb4; i++) {
        block = (struct shape far *)dos_alloc_bytes(sizeof(struct shape), DOS_ZERO_FILL);
        block->next = g_shape_free;
        g_shape_free = block;
    }
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
void game_teardown(int16_t really)
{
    struct shape far *node;            /* [bp-4] */
    struct shape far *next;            /* [bp-8] */
    struct queue_node *after;           /* [bp-0xa] */
    char code[40];                     /* [bp-0x32] */
    char msg[240];                     /* [bp-0x122] */
    struct queue_node *si;

    if (really == 0) {
        g_stop_requested = 1;
        return;
    }

    if (g_password_puzzle != 0) {
        read_password_line(g_password_puzzle, code);
        score_to_code(g_banked_score, code);
        strcpy(msg, g_messages.thanks_for_playing);
        strcat(msg, code);
    } else {
        msg[0] = 0;
    }

    /* Each free block's first four bytes are the far pointer to the next. */
    for (node = g_shape_free; node != NULL; node = next) {
        next = node->next;
        dos_free_far(node);
    }

    for (si = g_parts_free; si != 0; si = after) {
        after = si->next;
        free_far(si);
    }

    free_region_lists();
    free_all_part_bitmaps();

    free_bitmaps_thunk(g_icons_bmp);
    free_bitmaps_thunk(g_border_art);
    free_bitmaps_thunk(g_panel_art);
    free_bitmaps(g_cursor_art);

    close_font_slot(g_memo_font);

    free_far_block(g_pal_black);
    free_far_block(g_pal_sierra);
    free_far_block(g_pal_tim);

    stop_sequences(-2);
    remove_and_free_records(-2);
    shutdown_sound();

    close_file_record(g_tim_sx);
    free_archive_lists();

    remove_keyboard();
    shutdown_input();
    restore_video_mode();

    printf(msg);
    exit(0);
}
