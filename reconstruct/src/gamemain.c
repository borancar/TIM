/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The program**: `main`'s body, the bring-up and the teardown that prints
 * your password.
 *
 * In 1.11, image 0x0eccf..0x0f0ef, the first module of the code segment that
 * starts at 0x0ecc0 (1.00: segment 0dff, 0x0dfff..0x0e4be), and the first
 * object linked after the C startup: its data is its literal pool, DGROUP
 * 0x00aa..0x00f8, straight after C0M's. `rescfg.c` follows it, in the code
 * and in DGROUP. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x00aa..0x00f7
 */
#include <stdlib.h>
#include <string.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0eccf
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
 * 0x0eced
 *
 * The whole bring-up, in the original's order: refuse to run without enough
 * memory, read the configuration files, start the video driver, load the
 * palettes, the font and the pointer's bitmaps, start sound, install the
 * timer, and build two free lists.
 *
 * **1.11 measures the memory in thousands of bytes and keeps the answer**:
 * `dos_alloc_bytes` asked for 0xffffffff bytes with flags 0 - the "how much
 * is free" call - over 1000, as an `int`, into `g_memory_k`, which
 * `game_intro` later weighs each sound against. Under 297 (0x129) it prints
 * the two lines and exits; 1.00 compared the bytes with 282,000.
 *
 * Also new in 1.11: the BIOS keyboard flags at 0040:0017 lose Insert, Caps,
 * Num and Scroll Lock (`& 0x8f`) before anything else; RESOURCE.CFG is read
 * as text by `read_resource_cfg`, in a module of its own, which leaves the
 * sound device and module in DGROUP; the first 88 levels - 1.00's - are open
 * from the start; the master level defaults to 4 where 1.00's was 6; the
 * pointer is `newmouse.bmp`; the panel's art is `game_intro`'s to load; and
 * the 180 shape records are one zero-filled block from DOS, chained in place,
 * where 1.00 asked DOS for each.
 */
void game_startup(void)
{
    int16_t vm_ok;                     /* [bp-2] */
    int32_t free_bytes;                /* [bp-6] */
    uint8_t far *kb_flags;             /* [bp-0xa] */
    struct shape far *block;           /* [bp-0xe] */
    register int16_t i;                /* si */
    register struct queue_node *node;  /* di */

    _stklen = 0x800;

    kb_flags = LOW_MEMORY(0x417);
    *kb_flags &= 0x8f;

    free_bytes = DOS_ALLOC_BYTES(dos_alloc_bytes(0xffffffffUL, 0));
    g_memory_k = (int16_t)(free_bytes / 1000);
    if (g_memory_k < 0x129) {
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
    read_resource_cfg();

    if (read_tim_cfg() == 0) {
        g_furthest_level = 1;
        g_master_level = 4;
    }
    if (g_furthest_level < 0x58)
        g_furthest_level = 0x58;

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
    vm_set_display_lines(0x1d6);                /* 470 - the Dynamix screen */

    g_pal_tim = load_palette(WRITABLE_LITERAL("tim.pal"));
    g_pal_dynamix = load_palette(WRITABLE_LITERAL("dynamix.pal"));
    set_palette_pointer(g_pal_black = load_palette(WRITABLE_LITERAL("black.pal")));

    set_font(g_memo_font = load_font(WRITABLE_LITERAL("memofnt8.fnt")));

    g_cursor_art = load_bitmaps(WRITABLE_LITERAL("newmouse.bmp"));

    install_keyboard(0);

    start_sound(g_sound_device, g_sound_module, 0, (FILE *)WRITABLE_LITERAL("sx.ovl"));

    g_tim_sx = open_file_record(WRITABLE_LITERAL("tim.sx"));

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
     * word.
     */
    g_parts_free = g_parts_queue = 0;
    for (i = 0; i < 0x14; i++) {
        node = (struct queue_node *)calloc_far(1, sizeof(struct queue_node));
        node->next = g_parts_free;
        g_parts_free = node;
    }

    /*
     * And 180 shape records in one block from DOS, zero-filled, each pointing
     * at the next through the far pointer in its first four bytes; the last
     * one's stays null.
     */
    g_shapes_drawn = NULL;
    g_shape_free = block = (struct shape far *)dos_alloc_bytes(
        0xb4 * sizeof(struct shape), DOS_ZERO_FILL);
    for (i = 0; i < 0xb3; i++) {
        block->next = block + 1;
        block++;
    }
}

/*
 * 0x0ef9c
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
    struct queue_node *after;           /* [bp-2] */
    char code[40];                     /* [bp-0x2a] */
    char msg[240];                     /* [bp-0x11a] */
    register struct queue_node *si;

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

    /* The shape records are the one block `game_startup` took. */
    dos_free_far(g_shape_free);

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
    free_far_block(g_pal_dynamix);
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
    printf(g_newline);
    exit(0);
}
