/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
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
    si->x0 = 0x0;
    si->y0 = 0x0;
    si->x1 = 0x27f;
    si->y1 = 0x16f;
    si->code = 0x1000;
    si->hover = region_cursor_playfield;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 0x240;
    si->y0 = 0x0;
    si->x1 = 0x278;
    si->y1 = 0x3f;
    si->cursor = 0x1a;
    si->hover = region_cursor_bin_above;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 0x240;
    si->y0 = 0x43;
    si->x1 = 0x25b;
    si->y1 = 0x5a;
    si->code = 0x800;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->x0 = 0x260;
    si->y0 = 0x43;
    si->x1 = 0x27f;
    si->y1 = 0x5a;
    si->code = 0x400;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x0;
    si->x0 = 0x240;
    si->y0 = 0x64;
    si->x1 = 0x278;
    si->y1 = 0x90;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x1;
    si->x0 = 0x240;
    si->y0 = 0x91;
    si->x1 = 0x278;
    si->y1 = 0xc4;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x2;
    si->x0 = 0x240;
    si->y0 = 0xc5;
    si->x1 = 0x278;
    si->y1 = 0xf8;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x3;
    si->x0 = 0x240;
    si->y0 = 0xf9;
    si->x1 = 0x278;
    si->y1 = 0x12c;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x1000;
    si->slot = 0x4;
    si->x0 = 0x240;
    si->y0 = 0x12d;
    si->x1 = 0x278;
    si->y1 = 0x160;
    si->cursor = 0x2;
    si->code = 0x1000;
    si->hover = region_cursor_bin;
    si->click = region_click_bin;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xc000;
    si->x0 = 0x0;
    si->y0 = 0x0;
    si->x1 = 0x27f;
    si->y1 = 0x18f;
    si->code = 0x1000;
    si->link = g_regions_play;
    g_regions_play = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x110;
    si->y0 = 0x48;
    si->x1 = 0x210;
    si->y1 = 0xe8;
    si->cursor = 0x10;
    si->code = 0x8000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x3a;
    si->y0 = 0x5b;
    si->x1 = 0x4f;
    si->y1 = 0x7e;
    si->cursor = 0x10;
    si->code = 0x8000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0xd8;
    si->y0 = 0x60;
    si->x1 = 0xf0;
    si->y1 = 0x77;
    si->cursor = 0x15;
    si->code = 0x1000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x39;
    si->y0 = 0x86;
    si->x1 = 0x5f;
    si->y1 = 0xab;
    si->code = 0x400;
    si->hover = region_cursor_freeform;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x96;
    si->y0 = 0x8c;
    si->x1 = 0xbf;
    si->y1 = 0xa4;
    si->code = 0x100;
    si->hover = region_cursor_load;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x58;
    si->y0 = 0x5d;
    si->x1 = 0x6d;
    si->y1 = 0x6d;
    si->cursor = 0x11;
    si->code = 0x4000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x58;
    si->y0 = 0x6f;
    si->x1 = 0x6d;
    si->y1 = 0x7e;
    si->cursor = 0x11;
    si->code = 0x2000;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0xbc;
    si->y0 = 0x5c;
    si->x1 = 0xce;
    si->y1 = 0x7b;
    si->cursor = 0x12;
    si->code = 0x800;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x6d;
    si->y0 = 0x85;
    si->x1 = 0x8c;
    si->y1 = 0xa3;
    si->cursor = 0x13;
    si->code = 0x200;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0xc8;
    si->y0 = 0x8c;
    si->x1 = 0xf1;
    si->y1 = 0xa4;
    si->code = 0x80;
    si->hover = region_cursor_save;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x41;
    si->y0 = 0xc8;
    si->x1 = 0xe1;
    si->y1 = 0xf8;
    si->code = 0x40;
    si->hover = region_cursor_gravity;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x2;
    si->x0 = 0x41;
    si->y0 = 0x114;
    si->x1 = 0xe1;
    si->y1 = 0x144;
    si->code = 0x20;
    si->hover = region_cursor_air;
    si->link = g_regions_panel;
    g_regions_panel = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0x40;
    si->y0 = 0x56;
    si->x1 = 0xf8;
    si->y1 = 0x66;
    si->code = 0x4000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0x40;
    si->y0 = 0x7c;
    si->x1 = 0xb0;
    si->y1 = 0xf3;
    si->code = 0x2000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0x90;
    si->y0 = 0x10c;
    si->x1 = 0x100;
    si->y1 = 0x11c;
    si->code = 0x1000;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0xbc;
    si->y0 = 0x74;
    si->x1 = 0xdc;
    si->y1 = 0x94;
    si->code = 0x800;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0xbc;
    si->y0 = 0xe0;
    si->x1 = 0xdc;
    si->y1 = 0x100;
    si->code = 0x400;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0x40;
    si->y0 = 0x130;
    si->x1 = 0x90;
    si->y1 = 0x144;
    si->code = 0x200;
    si->link = g_regions_c;
    g_regions_c = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0xd000;
    si->x0 = 0xc0;
    si->y0 = 0x130;
    si->x1 = 0x110;
    si->y1 = 0x144;
    si->code = 0x100;
    si->link = g_regions_c;
    g_regions_c = si;

    g_region_kept_b = (si = (struct region *)(void *)
        calloc_far(1, sizeof(struct region)));
    si->mask = 0x8000;
    si->x0 = 0xc8;
    si->y0 = 0xd4;
    si->x1 = 0xc8;
    si->y1 = 0xe4;
    si->code = 0x4000;
    si->link = g_regions_b;
    g_regions_b = si;

    g_region_kept_a = (si = (struct region *)(void *)
        calloc_far(1, sizeof(struct region)));
    si->mask = 0x8000;
    si->x0 = 0x178;
    si->y0 = 0xd4;
    si->x1 = 0x178;
    si->y1 = 0xe4;
    si->code = 0x2000;
    si->link = g_regions_b;
    g_regions_b = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 0x30;
    si->y0 = 0x4c;
    si->x1 = 0x1c0;
    si->y1 = 0x11d;
    si->code = 0x4000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 0x1cc;
    si->y0 = 0x42;
    si->x1 = 0x1ec;
    si->y1 = 0x62;
    si->code = 0x2000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 0x1cc;
    si->y0 = 0x108;
    si->x1 = 0x1ec;
    si->y1 = 0x128;
    si->code = 0x1000;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8000;
    si->x0 = 0x90;
    si->y0 = 0x13c;
    si->x1 = 0x158;
    si->y1 = 0x14c;
    si->code = 0x800;
    si->link = g_regions_a;
    g_regions_a = si;

    si = (struct region *)(void *)calloc_far(1, sizeof(struct region));
    si->mask = 0x8800;
    si->x0 = 0x1f0;
    si->y0 = 0x12c;
    si->x1 = 0x218;
    si->y1 = 0x154;
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
