/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Setting up a game and a round**: the panels and score bars `game_setup`
 * loads, the round loop, and what each round builds and takes down.
 *
 * The third module of the original's **code segment 0dff**, image
 * 0x0ef19..0x0f0b0. Its data is only its literal pool, DGROUP
 * 0x25e8..0x260a. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d -O -Z
 * JUDGE: data 0x1fd2..0x1ff2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0fe81
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
 * What is kept is `gp_go.bmp` (1.00: `gp_menu.bmp`) at DGROUP 0x4ec9 and `score2.bmp` at 0x4ecd -
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
    struct bitmap **bar;

    cursor_redraw_off_thunk();
    bar = load_bitmaps(WRITABLE_LITERAL("score1.bmp"));

    g_vmds.page_dst = 0xa000;
    g_vmds.second_colour = g_vmds.fill_colour = 0;
    g_vmds.fill_enabled = 1;

    fill_rect(0, 0, 0x280, 0x50);

    if (bar != NULL) {                  /* 1.11 */
        draw_bitmap(bar[0], 3, 0, 0);
        draw_bitmap(bar[1], 0x107, 0, 0);
        draw_bitmap(bar[2], 0x1bb, 0, 0);

        free_bitmaps_thunk(bar);
    }
    heap_check_or_hang();               /* 1.11 */

    cursor_redraw_off_thunk();
    g_menu_bmp = load_bitmaps(WRITABLE_LITERAL("gp_go.bmp"));
    g_score2_bmp = load_bitmaps(WRITABLE_LITERAL("score2.bmp"));

    g_odometer_total = 0;
    g_round_number = 1;
    g_playing = 1;
    g_freeform = 0;
}

/*
 * 0x0ff45
 *
 * Give back the two bitmap lists the game keeps at DGROUP 0x4ecd and 0x4ec9,
 * in that order, through the driver's own thunk.
 */
void free_two_bitmap_lists(void)
{
    free_bitmaps_thunk(g_score2_bmp);
    free_bitmaps_thunk(g_menu_bmp);
}

/*
 * 0x0ff5e
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

    while (g_round_state != 0x200 && g_round_state != 1) {
        heap_check_or_hang();

        switch (g_round_state) {
        case 2:
            game_screen();
            break;
        case 0x2000:
            run_machine_loop();
            break;
        default:
            game_screen_loop();
            break;
        }
    }

    if (g_round_state == 0x200)
        finish_level();

    round_teardown();
}

/*
 * 0x0ffb2
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
    g_origin_c_x = g_origin_c_y = g_origin_b_x
        = g_origin_b_y = g_origin_x = g_origin_y = -8;
    g_round_unread = 0;

    heap_check_or_hang();

    if (g_freeform != 0) {
        g_placed_parts.next = g_placed_parts.prev
            = g_moving_parts.next = g_moving_parts.prev
            = g_held_parts.parts_bin.next = g_held_parts.parts_bin.prev = 0;
        build_part_list();
        reset_machine();
    } else {
        load_level(g_round_number);
    }

    g_level_settings.extent_y = g_level_settings.extent_x = -8;

    start_counters();
    reset_input_state();

    g_round_state = 2;
}

/*
 * 0x10021
 *
 * **Take the round down**, and it is one call: `free_all_lists`. Nothing else
 * happens - no saving, no drawing, no state reset. Everything a round owns is
 * on those lists, and everything else it touched belongs to the game rather
 * than to the round.
 *
 * It is a routine rather than a call because `game_round` ends in one place and
 * `game_screen`'s LOAD case and the freeform handlers end a round in others.
 */
void round_teardown(void)
{
    free_all_lists();
}
