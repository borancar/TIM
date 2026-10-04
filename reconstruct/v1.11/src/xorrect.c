/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **An XOR-drawn rectangle**, clipped, outline and optional fill.
 *
 * A module of its own in **code segment 0000** (`_TEXT`), image
 * 0x0a866..0x0aa8f, between gamefile.c and fstring.c. It was filed with
 * gamefile.c only because nothing said where else it belonged; its code
 * says: it is built without `-O` or `-Z` - every memory operand reloaded
 * after a conditional jump - where gamefile.c has both.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0a866
 *
 * **A rectangle drawn by XOR**, pixel by pixel through the driver's clipped
 * read and write: the outline in the second colour, and the inside in the
 * fill colour when filling is on - so drawing it twice takes it away again.
 * Drawn from the destination page into itself, the source page being set to
 * it for the while and put back after.
 *
 * With clipping on, each edge is drawn only if it is inside the clip box, and
 * the rectangle is cut down to the box. **A cut-down left or top edge is set
 * to the clip-enabled byte, 1, not to the clip box's edge**: that is what the
 * image does, and it is transcribed so.
 *
 * Nothing in the image calls it, near or far. It is transcribed because the
 * module holds it. The name is ours.
 */
void draw_xor_rect(register int16_t x, int16_t y, int16_t w, int16_t h)
{
    register int16_t si;
    int16_t yy;                         /* [bp-2] */
    int16_t outline;                    /* [bp-4] */
    int16_t fill;                       /* [bp-6] */
    int16_t end;                        /* [bp-8] */
    uint16_t saved;                     /* [bp-0xa] */
    int16_t left_in;                    /* [bp-0xc] */
    int16_t right_in;                   /* [bp-0xe] */
    int16_t top_in;                     /* [bp-0x10] */
    int16_t bottom_in;                  /* [bp-0x12] */
    int16_t x_end;                      /* [bp-0x14] */
    int16_t y_end;                      /* [bp-0x16] */

    if (!g_vmds.second_colour && !g_vmds.fill_colour)
        return;
    saved = g_vmds.page_src;
    g_vmds.page_src = g_vmds.page_dst;
    outline = g_vmds.second_colour;
    fill = g_vmds.fill_colour;
    if (g_vmds.clip_enabled) {
        end = x + w - 1;
        left_in = x >= g_vmds.clip_left && x <= g_vmds.clip_right;
        right_in = end >= g_vmds.clip_left && end <= g_vmds.clip_right;
        if (x < g_vmds.clip_left) {
            w -= g_vmds.clip_left - x;
            x = g_vmds.clip_enabled;
        }
        if (x > g_vmds.clip_right) {
            w -= x - g_vmds.clip_right;
            x = g_vmds.clip_right;
        }
        end = y + h - 1;
        top_in = y >= g_vmds.clip_top && y <= g_vmds.clip_bottom;
        bottom_in = end >= g_vmds.clip_top && end <= g_vmds.clip_bottom;
        if (y < g_vmds.clip_top) {
            h -= g_vmds.clip_top - y;
            y = g_vmds.clip_enabled;
        }
        if (y > g_vmds.clip_bottom) {
            h -= y - g_vmds.clip_bottom;
            y = g_vmds.clip_bottom;
        }
    } else
        left_in = right_in = top_in = bottom_in = 1;

    x_end = x + w - 1;
    y_end = y + h - 1;
    if (g_vmds.fill_enabled)
        for (yy = y + 1; yy < y_end; yy++)
            for (si = x + 1; si < x_end; si++)
                plot_pixel_clipped(si, yy, read_pixel_clipped(si, yy) ^ fill);
    for (si = y; si <= y_end; si++) {
        if (left_in)
            plot_pixel_clipped(x, si, read_pixel_clipped(x, si) ^ outline);
        if (right_in)
            plot_pixel_clipped(x_end, si, read_pixel_clipped(x_end, si) ^ outline);
    }
    for (si = x + 1; si < x_end; si++) {
        if (top_in)
            plot_pixel_clipped(si, y, read_pixel_clipped(si, y) ^ outline);
        if (bottom_in)
            plot_pixel_clipped(si, y_end, read_pixel_clipped(si, y_end) ^ outline);
    }
    g_vmds.page_src = saved;
}
