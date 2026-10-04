/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The pointer's regions**: which rectangle of the screen the pointer is
 * in, and the thirty-six regions built at start-up and freed at the end.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x08546..0x08f27,
 * split out of seg0000.c on 2026-09-27. **Both ends are ours.** Its start
 * is somewhere after 0x08510: `free_region_lists` calls `checked_free`
 * through TLINK's `nop / push cs / call`, so those two are in different
 * files, and the split is put where the subject changes. It ends where
 * crtc.c's two routines begin, whose prologue no compiler route reproduces.
 * Nothing here names `_DATA`.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x08546
 *
 * Walk a list of screen regions and act on the one the pointer is in. The list
 * is one of the five `build_screen_regions` built, and its records are the
 * 0x1a-byte ones from there: a link at +0, a mask of which screens the region
 * belongs to at +2, its rectangle at +6 through +0xc, a cursor number at +0xe,
 * a screen to switch to at +0x10, and **two far function pointers**, at +0x12
 * for entering the region and +0x16 for clicking in it.
 *
 * DGROUP 0x4e6b is the screen the game is on, and a region whose mask does not
 * have that bit is skipped without its rectangle even being looked at. The
 * pointer's position is DGROUP 0x5782 and 0x5784, and 0x5774 being 2 is the
 * button.
 *
 * The walk **stops at the region it acts on** - `si` is zeroed rather than
 * followed - so the first match wins and the ones after it are never
 * considered. Running off the end of the list without a match sets the cursor
 * back to 0.
 *
 * The two far calls are the relocated pointers `build_screen_regions` files in;
 * the port cannot call through a guest far pointer and dispatches on the value
 * instead, which is why an unexpected one aborts rather than being ignored.
 *
 * **The argument is the first record, not the word that holds it.** `mov si,
 * [bp+6]` uses it directly, and every one of the seven call sites pushes
 * `word ptr [0x4e71]` and its neighbours - the value in the word, never the
 * word's address. The port used to dereference here as well, and the two
 * conventions cancelled for the two callers that passed the address: the other
 * five skipped the head of their list.
 */
void regions_handle_pointer(register struct region *si)
{
    while (si != 0) {
        if ((si->mask & g_round_state)
            && si->x0 <= g_pointer.pointer_x
            && si->x1 >= g_pointer.pointer_x
            && si->y0 <= g_pointer.pointer_y
            && si->y1 >= g_pointer.pointer_y) {
            if (si->hover != NULL)
                si->hover(si);
            select_cursor(si->cursor);
            if (g_pointer.button_left == 2) {
                if (si->click != NULL)
                    si->click(si);
                g_round_state = si->code;
            }
            /* acted on: the rest of the list is not looked at */
            si = 0;
        }
        if (si != 0 && (si = si->link) == 0)
            select_cursor(0);
    }
}

/*
 * 0x085c9
 *
 * Build the game's screen regions: thirty-six records of 0x1a bytes off the
 * near heap, each pushed onto the front of one of five lists whose heads are
 * the words at DGROUP 0x4e71 through 0x4e79. Two of them are also remembered
 * on their own, at 0x4e6d and 0x4e6f, as well as going on a list.
 *
 * 2,283 bytes of straight-line stores, and transcribed as the straight line it
 * is: every statement below is one store of the image's, in the image's order,
 * zeros included - `calloc_far` has already zeroed the record, and the
 * original writes some of them again. Generated from the disassembly, one
 * statement per instruction group, and judged against it.
 *
 * The allocation's size is `sizeof(struct region)`, which is 0x1a under
 * Borland; the host's far pointers are wider.
 */
void build_screen_regions(void)
{
    register struct region *si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 0;
    si->y0 = 0;
    si->x1 = 639;
    si->y1 = 367;
    si->code = 0x1000;
    si->hover = region_cursor_playfield;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 576;
    si->y0 = 0;
    si->x1 = 632;
    si->y1 = 63;
    si->cursor = 0x1a;
    si->hover = region_cursor_bin_above;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 576;
    si->y0 = 67;
    si->x1 = 603;
    si->y1 = 90;
    si->code = 0x800;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 608;
    si->y0 = 67;
    si->x1 = 639;
    si->y1 = 90;
    si->code = 0x400;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x0;
    si->x0 = 576;
    si->y0 = 100;
    si->x1 = 632;
    si->y1 = 144;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x1;
    si->x0 = 576;
    si->y0 = 145;
    si->x1 = 632;
    si->y1 = 196;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x2;
    si->x0 = 576;
    si->y0 = 197;
    si->x1 = 632;
    si->y1 = 248;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x3;
    si->x0 = 576;
    si->y0 = 249;
    si->x1 = 632;
    si->y1 = 300;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x4;
    si->x0 = 576;
    si->y0 = 301;
    si->x1 = 632;
    si->y1 = 352;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xc000;
    si->x0 = 0;
    si->y0 = 0;
    si->x1 = 639;
    si->y1 = 399;
    si->code = 0x1000;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 272;
    si->y0 = 72;
    si->x1 = 528;
    si->y1 = 232;
    si->cursor = 0x10;
    si->code = 0x8000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 58;
    si->y0 = 91;
    si->x1 = 79;
    si->y1 = 126;
    si->cursor = 0x10;
    si->code = 0x8000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 216;
    si->y0 = 96;
    si->x1 = 240;
    si->y1 = 119;
    si->cursor = 0x15;
    si->code = 0x1000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 57;
    si->y0 = 134;
    si->x1 = 95;
    si->y1 = 171;
    si->code = 0x400;
    si->hover = region_cursor_freeform;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 150;
    si->y0 = 140;
    si->x1 = 191;
    si->y1 = 164;
    si->code = 0x100;
    si->hover = region_cursor_load;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 88;
    si->y0 = 93;
    si->x1 = 109;
    si->y1 = 109;
    si->cursor = 0x11;
    si->code = 0x4000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 88;
    si->y0 = 111;
    si->x1 = 109;
    si->y1 = 126;
    si->cursor = 0x11;
    si->code = 0x2000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 188;
    si->y0 = 92;
    si->x1 = 206;
    si->y1 = 123;
    si->cursor = 0x12;
    si->code = 0x800;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 109;
    si->y0 = 133;
    si->x1 = 140;
    si->y1 = 163;
    si->cursor = 0x13;
    si->code = 0x200;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 200;
    si->y0 = 140;
    si->x1 = 241;
    si->y1 = 164;
    si->code = 0x80;
    si->hover = region_cursor_save;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 65;
    si->y0 = 200;
    si->x1 = 225;
    si->y1 = 248;
    si->code = 0x40;
    si->hover = region_cursor_gravity;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 65;
    si->y0 = 276;
    si->x1 = 225;
    si->y1 = 324;
    si->code = 0x20;
    si->hover = region_cursor_air;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 64;
    si->y0 = 86;
    si->x1 = 248;
    si->y1 = 102;
    si->code = 0x4000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 64;
    si->y0 = 124;
    si->x1 = 176;
    si->y1 = 243;
    si->code = 0x2000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 144;
    si->y0 = 268;
    si->x1 = 256;
    si->y1 = 284;
    si->code = 0x1000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 188;
    si->y0 = 116;
    si->x1 = 220;
    si->y1 = 148;
    si->code = 0x800;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 188;
    si->y0 = 224;
    si->x1 = 220;
    si->y1 = 256;
    si->code = 0x400;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 64;
    si->y0 = 304;
    si->x1 = 144;
    si->y1 = 324;
    si->code = 0x200;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 192;
    si->y0 = 304;
    si->x1 = 272;
    si->y1 = 324;
    si->code = 0x100;
    si->link = g_regions_c;
    g_regions_c = si;

    g_region_kept_b = (si = (struct region *)(void *)
        calloc_far(1, sizeof(struct region)));
    si->mask = 0x8000;
    si->x0 = 200;
    si->y0 = 212;
    si->x1 = 200;
    si->y1 = 228;
    si->code = 0x4000;
    si->link = g_regions_b;
    g_regions_b = si;

    g_region_kept_a = (si = (struct region *)(void *)
        calloc_far(1, sizeof(struct region)));
    si->mask = 0x8000;
    si->x0 = 376;
    si->y0 = 212;
    si->x1 = 376;
    si->y1 = 228;
    si->code = 0x2000;
    si->link = g_regions_b;
    g_regions_b = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 48;
    si->y0 = 76;
    si->x1 = 448;
    si->y1 = 285;
    si->code = 0x4000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 460;
    si->y0 = 66;
    si->x1 = 492;
    si->y1 = 98;
    si->code = 0x2000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 460;
    si->y0 = 264;
    si->x1 = 492;
    si->y1 = 296;
    si->code = 0x1000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8000;
    si->x0 = 144;
    si->y0 = 316;
    si->x1 = 344;
    si->y1 = 332;
    si->code = 0x800;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 496;
    si->y0 = 300;
    si->x1 = 536;
    si->y1 = 340;
    si->code = 0x400;
    si->link = g_regions_a;
    g_regions_a = si;
}

/*
 * 0x08eb5
 *
 * Free all five lists of screen regions, each walked to its end and each
 * record handed to `checked_free`.
 *
 * **The order is 0x4e73, 0x4e71, 0x4e75, 0x4e77, 0x4e79** - the second list
 * before the first, which is not a mistake anyone would make writing it out
 * and is left as the original has it. The link is read into `di` before the
 * record is freed, because after the free it is not there to read.
 */
void free_region_lists(void)
{
    register struct region *si;
    register struct region *di;

    for (si = g_regions_b; si != 0; si = di) {
        di = si->link;
        checked_free((uint8_t *)si);
    }
    for (si = g_regions_a; si != 0; si = di) {
        di = si->link;
        checked_free((uint8_t *)si);
    }
    for (si = g_regions_c; si != 0; si = di) {
        di = si->link;
        checked_free((uint8_t *)si);
    }
    for (si = g_regions_panel; si != 0; si = di) {
        di = si->link;
        checked_free((uint8_t *)si);
    }
    for (si = g_regions_play; si != 0; si = di) {
        di = si->link;
        checked_free((uint8_t *)si);
    }
}
