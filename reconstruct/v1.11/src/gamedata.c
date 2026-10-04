/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game's tables: the part kinds and their drawing tables** - DGROUP
 * 0x0222..0x14eb in 1.11, data and no code - **and the game state that
 * starts out zero**, its `_BSS`.
 *
 * Where it stands in the link is measured: TLINK lays each segment out in
 * object order, and this data comes after rescfg.c's and before
 * messages.c's. In 1.00 this file held the message strings too, and nothing
 * was padded to show whether they were one object or two; 1.11 pads the
 * byte at 0x14eb, so the strings are an object of their own (messages.c).
 * The objects came out of dgroup.c, their initialisers made positional
 * (tools/c89init.py) because Borland C++ has no designators, and the two
 * all-zero tables given `= { 0 }` so they stay in `_DATA`.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x0222..0x14eb
 */
/* this file defines what dgroup.h declares `extern` for the rest: see the
   `_BSS` below */
#define GAMEDATA_C
/* Named before <dos.h>, which dgroup.h includes and which names `_stklen`:
   Borland lays `_BSS` out last mention first, and these two follow
   `_stklen` in the image. */
extern struct game_directories g_game_directories;
extern char g_picked_machine[0xd];
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

uint16_t g_master_level_ok[7] = { 0x0000, 0x0003, 0x0005, 0x0008, 0x000a, 0x000d, 0x000f };

/*
 * DGROUP 0x0230 and 0x0264 in 1.11, new there: **the sounds `game_intro`
 * loads ahead of time, and the memory each needs**, in thousands of bytes of
 * the largest free block (`g_memory_k`). Twenty-six of each, and read from 1
 * - `game_intro` walks 1..0x14 before the title and 0x15..0x1a after the
 * credits and takes entry `si - 1` - so the first twenty are the effects, the
 * rest the tunes. A need of 0 always loads; 999 never does.
 */
int16_t g_preload_sound_ids[26] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    51, 54, 58, 61, 62, 64,
};
int16_t g_preload_sound_need[26] = {
    0, 0, 0, 999, 999, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 317, 343, 0,
    0, 327, 333, 0, 0, 0,
};

/* The draw step `draw_part` fills in for a part whose kind has no table. */
struct draw_step g_default_draw_step = { 0, 0, { 0, 0xff }, { { 0 } } };   /* DGROUP 0x0298 */

/*
 * DGROUP 0x02a7..0x14eb in 1.11 (1.00: 0x0133..0x0ea6) - **the kinds'
 * drawing tables**, which `g_part_kinds` points into and nothing else does.
 * A kind that draws in steps has a run of `draw_step`s, then three tables
 * indexed by form: the first step (`bitmaps2`), the size (`sizes`) and the
 * hot spot (`hotspots`); a kind that draws one bitmap per form has at most
 * the last two. The boundaries are where the kind records point, and they
 * tile the range with nothing left over. The `next` links and the table
 * entries are near pointers, as the image has them. Written by
 * `tools/kindtables.py --tables --end 0x14eb` from the image.
 */
struct draw_step g_jack_in_the_box_draw_steps[19] = {   /* DGROUP 0x02a7 */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x04, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x06, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x07, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 8, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x08, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x09, 0xff },    /* frame */
        { { 0, 3 }, { 0 }, { 9, 8 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0a, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { 0, -21 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0b, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { 1, -34 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0c, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -6, -59 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0d, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0e, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -6, -30 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0f, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x10, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x11, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x12, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x13, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x14, 0x02, 0xff },    /* frame */
        { { 0, 3 }, { -4, -25 }, { 9, 9 } }    /* offset */
    },
};

struct draw_step *g_jack_in_the_box_form_steps[19] = {   /* DGROUP 0x03c4 */
    NEAR_AT(0x02a7, &g_jack_in_the_box_draw_steps[0]),
    NEAR_AT(0x02b6, &g_jack_in_the_box_draw_steps[1]),
    NEAR_AT(0x02c5, &g_jack_in_the_box_draw_steps[2]),
    NEAR_AT(0x02d4, &g_jack_in_the_box_draw_steps[3]),
    NEAR_AT(0x02e3, &g_jack_in_the_box_draw_steps[4]),
    NEAR_AT(0x02f2, &g_jack_in_the_box_draw_steps[5]),
    NEAR_AT(0x0301, &g_jack_in_the_box_draw_steps[6]),
    NEAR_AT(0x0310, &g_jack_in_the_box_draw_steps[7]),
    NEAR_AT(0x031f, &g_jack_in_the_box_draw_steps[8]),
    NEAR_AT(0x032e, &g_jack_in_the_box_draw_steps[9]),
    NEAR_AT(0x033d, &g_jack_in_the_box_draw_steps[10]),
    NEAR_AT(0x034c, &g_jack_in_the_box_draw_steps[11]),
    NEAR_AT(0x035b, &g_jack_in_the_box_draw_steps[12]),
    NEAR_AT(0x036a, &g_jack_in_the_box_draw_steps[13]),
    NEAR_AT(0x0379, &g_jack_in_the_box_draw_steps[14]),
    NEAR_AT(0x0388, &g_jack_in_the_box_draw_steps[15]),
    NEAR_AT(0x0397, &g_jack_in_the_box_draw_steps[16]),
    NEAR_AT(0x03a6, &g_jack_in_the_box_draw_steps[17]),
    NEAR_AT(0x03b5, &g_jack_in_the_box_draw_steps[18]),
};

struct point16 g_jack_in_the_box_form_sizes[19] = {   /* DGROUP 0x03ea */
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 35, 53 },
    { 35, 66 },
    { 39, 91 },
    { 36, 57 },
    { 38, 62 },
    { 37, 57 },
    { 41, 57 },
    { 40, 57 },
    { 36, 57 },
    { 36, 57 },
    { 36, 57 },
};

struct offset8 g_jack_in_the_box_hot_spots[19] = {   /* DGROUP 0x0436 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, -21 },
    { 0, -34 },
    { -6, -60 },
    { -4, -25 },
    { -6, -30 },
    { -4, -25 },
    { -4, -25 },
    { -4, -25 },
    { -4, -25 },
    { -4, -25 },
    { -4, -25 },
};

struct draw_step g_bob_the_fish_draw_steps[23] = {   /* DGROUP 0x045c */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x0c, 0xff },    /* frame */
        { { 0 }, { 10, 11 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x02, 0x0c, 0xff },    /* frame */
        { { 0 }, { 15, 9 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x03, 0x0c, 0xff },    /* frame */
        { { 0 }, { 26, 16 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x04, 0x0c, 0xff },    /* frame */
        { { 0 }, { 31, 17 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x05, 0x0c, 0xff },    /* frame */
        { { 0 }, { 26, 16 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x06, 0x0c, 0xff },    /* frame */
        { { 0 }, { 10, 11 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x07, 0x0c, 0xff },    /* frame */
        { { 0 }, { 7, 11 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x08, 0x0c, 0xff },    /* frame */
        { { 0 }, { 6, 10 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x09, 0x0c, 0xff },    /* frame */
        { { 0 }, { 7, 16 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0a, 0x0c, 0xff },    /* frame */
        { { 0 }, { 8, 16 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0b, 0x0c, 0xff },    /* frame */
        { { 0 }, { 6, 10 }, { 10, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x0d, 0xff, 0xff, 0xff },    /* frame */
        { { -16, 8 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x0e, 0xff, 0xff, 0xff },    /* frame */
        { { -19, 15 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x0f, 0xff, 0xff, 0xff },    /* frame */
        { { -25, 19 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x11, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 5, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x12, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 5, 31 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x13, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 3, 29 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x14, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 5, 29 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x15, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { -5, 27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x16, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { -5, 22 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x17, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 1, 31 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x18, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 5, 36 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x19, 0xff, 0xff },    /* frame */
        { { -28, 25 }, { 5, 38 } }    /* offset */
    },
};

struct draw_step *g_bob_the_fish_form_steps[23] = {   /* DGROUP 0x05b5 */
    NEAR_AT(0x045c, &g_bob_the_fish_draw_steps[0]),
    NEAR_AT(0x046b, &g_bob_the_fish_draw_steps[1]),
    NEAR_AT(0x047a, &g_bob_the_fish_draw_steps[2]),
    NEAR_AT(0x0489, &g_bob_the_fish_draw_steps[3]),
    NEAR_AT(0x0498, &g_bob_the_fish_draw_steps[4]),
    NEAR_AT(0x04a7, &g_bob_the_fish_draw_steps[5]),
    NEAR_AT(0x04b6, &g_bob_the_fish_draw_steps[6]),
    NEAR_AT(0x04c5, &g_bob_the_fish_draw_steps[7]),
    NEAR_AT(0x04d4, &g_bob_the_fish_draw_steps[8]),
    NEAR_AT(0x04e3, &g_bob_the_fish_draw_steps[9]),
    NEAR_AT(0x04f2, &g_bob_the_fish_draw_steps[10]),
    NEAR_AT(0x0501, &g_bob_the_fish_draw_steps[11]),
    NEAR_AT(0x0510, &g_bob_the_fish_draw_steps[12]),
    NEAR_AT(0x051f, &g_bob_the_fish_draw_steps[13]),
    NEAR_AT(0x052e, &g_bob_the_fish_draw_steps[14]),
    NEAR_AT(0x053d, &g_bob_the_fish_draw_steps[15]),
    NEAR_AT(0x054c, &g_bob_the_fish_draw_steps[16]),
    NEAR_AT(0x055b, &g_bob_the_fish_draw_steps[17]),
    NEAR_AT(0x056a, &g_bob_the_fish_draw_steps[18]),
    NEAR_AT(0x0579, &g_bob_the_fish_draw_steps[19]),
    NEAR_AT(0x0588, &g_bob_the_fish_draw_steps[20]),
    NEAR_AT(0x0597, &g_bob_the_fish_draw_steps[21]),
    NEAR_AT(0x05a6, &g_bob_the_fish_draw_steps[22]),
};

struct point16 g_bob_the_fish_form_sizes[23] = {   /* DGROUP 0x05e3 */
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 48, 48 },
    { 88, 43 },
    { 88, 38 },
    { 96, 37 },
    { 104, 32 },
    { 104, 32 },
    { 104, 32 },
    { 104, 32 },
    { 104, 32 },
    { 104, 35 },
    { 104, 32 },
    { 104, 32 },
    { 104, 32 },
};

struct offset8 g_bob_the_fish_hot_spots[23] = {   /* DGROUP 0x063f */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { -16, 8 },
    { -19, 15 },
    { -25, 19 },
    { -28, 25 },
    { -28, 25 },
    { -28, 25 },
    { -28, 25 },
    { -28, 25 },
    { -28, 22 },
    { -28, 25 },
    { -28, 25 },
    { -28, 25 },
};

struct draw_step g_cannon_draw_steps[15] = {   /* DGROUP 0x066d */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0xff, 0xff },    /* frame */
        { { 0 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0a, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { -9, -6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0b, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { -9, -6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0c, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { -6, -5 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0d, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { -7, -3 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0e, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { -6, -2 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x01, 0x09, 0xff, 0xff },    /* frame */
        { { -7, -8 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0x09, 0xff, 0xff },    /* frame */
        { { -8, -11 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x04, 0xff, 0xff, 0xff },    /* frame */
        { { 83, -33 } }    /* offset */
    },
    {
        NEAR_AT(0x06e5, &g_cannon_draw_steps[8]),    /* next */
        0x04,    /* level */
        { 0x03, 0x09, 0xff, 0xff },    /* frame */
        { { -2, -3 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x06, 0xff, 0xff, 0xff },    /* frame */
        { { 101, -46 } }    /* offset */
    },
    {
        NEAR_AT(0x0703, &g_cannon_draw_steps[10]),    /* next */
        0x04,    /* level */
        { 0x05, 0x09, 0xff, 0xff },    /* frame */
        { { -2, -7 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x08, 0xff, 0xff, 0xff },    /* frame */
        { { 127, -33 } }    /* offset */
    },
    {
        NEAR_AT(0x0721, &g_cannon_draw_steps[12]),    /* next */
        0x04,    /* level */
        { 0x07, 0x09, 0xff, 0xff },    /* frame */
        { { -3, -3 }, { 9, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0f, 0xff },    /* frame */
        { { 0 }, { 9, 13 }, { 0, 2 } }    /* offset */
    },
};

struct draw_step *g_cannon_form_steps[12] = {   /* DGROUP 0x074e */
    NEAR_AT(0x066d, &g_cannon_draw_steps[0]),
    NEAR_AT(0x067c, &g_cannon_draw_steps[1]),
    NEAR_AT(0x068b, &g_cannon_draw_steps[2]),
    NEAR_AT(0x069a, &g_cannon_draw_steps[3]),
    NEAR_AT(0x06a9, &g_cannon_draw_steps[4]),
    NEAR_AT(0x06b8, &g_cannon_draw_steps[5]),
    NEAR_AT(0x06c7, &g_cannon_draw_steps[6]),
    NEAR_AT(0x06d6, &g_cannon_draw_steps[7]),
    NEAR_AT(0x06f4, &g_cannon_draw_steps[9]),
    NEAR_AT(0x0712, &g_cannon_draw_steps[11]),
    NEAR_AT(0x0730, &g_cannon_draw_steps[13]),
    NEAR_AT(0x073f, &g_cannon_draw_steps[14]),
};

struct point16 g_cannon_form_sizes[12] = {   /* DGROUP 0x0766 */
    { 64, 52 },
    { 73, 58 },
    { 73, 58 },
    { 70, 57 },
    { 71, 55 },
    { 70, 54 },
    { 53, 60 },
    { 61, 63 },
    { 194, 84 },
    { 210, 94 },
    { 199, 84 },
    { 64, 52 },
};

struct offset8 g_cannon_hot_spots[12] = {   /* DGROUP 0x0796 */
    { 0 },
    { -9, -6 },
    { -9, -6 },
    { -6, -5 },
    { -7, -3 },
    { -6, -2 },
    { -7, -8 },
    { -8, -11 },
    { -2, -33 },
    { -2, -46 },
    { -3, -33 },
    { 0 },
};

struct draw_step g_dynamite_draw_steps[6] = {   /* DGROUP 0x07ae */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 40, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0 }, { 39, 10 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x03, 0xff, 0xff },    /* frame */
        { { 0 }, { 39, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 37, 13 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x05, 0xff, 0xff },    /* frame */
        { { 0 }, { 38, 12 } }    /* offset */
    },
};

struct draw_step *g_dynamite_form_steps[6] = {   /* DGROUP 0x0808 */
    NEAR_AT(0x07ae, &g_dynamite_draw_steps[0]),
    NEAR_AT(0x07bd, &g_dynamite_draw_steps[1]),
    NEAR_AT(0x07cc, &g_dynamite_draw_steps[2]),
    NEAR_AT(0x07db, &g_dynamite_draw_steps[3]),
    NEAR_AT(0x07ea, &g_dynamite_draw_steps[4]),
    NEAR_AT(0x07f9, &g_dynamite_draw_steps[5]),
};

struct point16 g_dynamite_form_sizes[6] = {   /* DGROUP 0x0814 */
    { 48, 28 },
    { 56, 28 },
    { 56, 28 },
    { 56, 28 },
    { 56, 28 },
    { 56, 28 },
};

struct offset8 g_dynamite_hot_spots[6] = {   /* DGROUP 0x082c */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
};

struct draw_step g_electric_plug_draw_steps[8] = {   /* DGROUP 0x0838 */
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 8, 8 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0x03 },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 4 }, { 29, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0 }, { 8, 8 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0xff },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0xff },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0x03 },    /* frame */
        { { 0 }, { 8, 8 }, { 29, 4 }, { 29, 18 } }    /* offset */
    },
};

struct draw_step *g_electric_plug_form_steps[8] = {   /* DGROUP 0x08b0 */
    NEAR_AT(0x0838, &g_electric_plug_draw_steps[0]),
    NEAR_AT(0x0847, &g_electric_plug_draw_steps[1]),
    NEAR_AT(0x0856, &g_electric_plug_draw_steps[2]),
    NEAR_AT(0x0865, &g_electric_plug_draw_steps[3]),
    NEAR_AT(0x0874, &g_electric_plug_draw_steps[4]),
    NEAR_AT(0x0883, &g_electric_plug_draw_steps[5]),
    NEAR_AT(0x0892, &g_electric_plug_draw_steps[6]),
    NEAR_AT(0x08a1, &g_electric_plug_draw_steps[7]),
};

struct point16 g_electric_plug_form_sizes[8] = {   /* DGROUP 0x08c0 */
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
    { 48, 32 },
};

struct draw_step g_dynamite_plunger_draw_steps[3] = {   /* DGROUP 0x08e0 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x01, 0xff },    /* frame */
        { { 0, 19 }, { 103, 0 }, { 40, 16 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x01, 0xff },    /* frame */
        { { 0, 19 }, { 103, 5 }, { 40, 16 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0x01, 0xff, 0xff },    /* frame */
        { { 103, 10 }, { 40, 16 } }    /* offset */
    },
};

struct draw_step *g_dynamite_plunger_form_steps[3] = {   /* DGROUP 0x090d */
    NEAR_AT(0x08e0, &g_dynamite_plunger_draw_steps[0]),
    NEAR_AT(0x08ef, &g_dynamite_plunger_draw_steps[1]),
    NEAR_AT(0x08fe, &g_dynamite_plunger_draw_steps[2]),
};

struct point16 g_dynamite_plunger_form_sizes[3] = {   /* DGROUP 0x0913 */
    { 135, 48 },
    { 135, 46 },
    { 135, 41 },
};

struct offset8 g_dynamite_plunger_hot_spots[3] = {   /* DGROUP 0x091f */
    { 0 },
    { 0 },
    { 0 },
};

struct draw_step g_fan_draw_steps[4] = {   /* DGROUP 0x0925 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0, 8 }, { 16, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0, 8 }, { 16, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x03, 0xff, 0xff },    /* frame */
        { { 0, 8 }, { 16, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0, 8 }, { 16, 0 } }    /* offset */
    },
};

struct draw_step *g_fan_form_steps[4] = {   /* DGROUP 0x0961 */
    NEAR_AT(0x0925, &g_fan_draw_steps[0]),
    NEAR_AT(0x0934, &g_fan_draw_steps[1]),
    NEAR_AT(0x0943, &g_fan_draw_steps[2]),
    NEAR_AT(0x0952, &g_fan_draw_steps[3]),
};

struct point16 g_fan_form_sizes[4] = {   /* DGROUP 0x0969 */
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
    { 32, 32 },
};

struct draw_step g_generator_draw_steps[16] = {   /* DGROUP 0x0979 */
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 21, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0 }, { 21, -6 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0xff, 0xff },    /* frame */
        { { 0 }, { 21, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 21, -5 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, 0 }, { 5, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, -6 }, { 5, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, 1 }, { 5, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, -5 }, { 5, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, 0 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, -6 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, 1 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0xff },    /* frame */
        { { 0 }, { 21, -5 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0x05 },    /* frame */
        { { 0 }, { 21, 0 }, { 5, 4 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0x05 },    /* frame */
        { { 0 }, { 21, -6 }, { 5, 4 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0x05 },    /* frame */
        { { 0 }, { 21, 1 }, { 5, 4 }, { 5, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0x05 },    /* frame */
        { { 0 }, { 21, -5 }, { 5, 4 }, { 5, 18 } }    /* offset */
    },
};

struct draw_step *g_generator_form_steps[16] = {   /* DGROUP 0x0a69 */
    NEAR_AT(0x0979, &g_generator_draw_steps[0]),
    NEAR_AT(0x0988, &g_generator_draw_steps[1]),
    NEAR_AT(0x0997, &g_generator_draw_steps[2]),
    NEAR_AT(0x09a6, &g_generator_draw_steps[3]),
    NEAR_AT(0x09b5, &g_generator_draw_steps[4]),
    NEAR_AT(0x09c4, &g_generator_draw_steps[5]),
    NEAR_AT(0x09d3, &g_generator_draw_steps[6]),
    NEAR_AT(0x09e2, &g_generator_draw_steps[7]),
    NEAR_AT(0x09f1, &g_generator_draw_steps[8]),
    NEAR_AT(0x0a00, &g_generator_draw_steps[9]),
    NEAR_AT(0x0a0f, &g_generator_draw_steps[10]),
    NEAR_AT(0x0a1e, &g_generator_draw_steps[11]),
    NEAR_AT(0x0a2d, &g_generator_draw_steps[12]),
    NEAR_AT(0x0a3c, &g_generator_draw_steps[13]),
    NEAR_AT(0x0a4b, &g_generator_draw_steps[14]),
    NEAR_AT(0x0a5a, &g_generator_draw_steps[15]),
};

struct point16 g_generator_form_sizes[16] = {   /* DGROUP 0x0a89 */
    { 80, 32 },
    { 80, 38 },
    { 80, 32 },
    { 80, 37 },
    { 80, 32 },
    { 80, 38 },
    { 80, 32 },
    { 80, 37 },
    { 80, 32 },
    { 80, 38 },
    { 80, 32 },
    { 80, 37 },
    { 80, 32 },
    { 80, 38 },
    { 80, 32 },
    { 80, 37 },
};

struct offset8 g_generator_hot_spots[16] = {   /* DGROUP 0x0ac9 */
    { 0 },
    { 0, -6 },
    { 0 },
    { 0, -5 },
    { 0 },
    { 0, -6 },
    { 0 },
    { 0, -5 },
    { 0 },
    { 0, -6 },
    { 0 },
    { 0, -5 },
    { 0 },
    { 0, -6 },
    { 0 },
    { 0, -5 },
};

struct draw_step g_gun_draw_steps[7] = {   /* DGROUP 0x0ae9 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { -1, -5 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { -2, -3 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x03, 0xff, 0xff, 0xff },    /* frame */
        { { -15, -6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x04, 0xff, 0xff, 0xff },    /* frame */
        { { -11, -3 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0xff, 0xff },    /* frame */
        { { -2, 0 }, { 64, -12 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
};

struct draw_step *g_gun_form_steps[7] = {   /* DGROUP 0x0b52 */
    NEAR_AT(0x0ae9, &g_gun_draw_steps[0]),
    NEAR_AT(0x0af8, &g_gun_draw_steps[1]),
    NEAR_AT(0x0b07, &g_gun_draw_steps[2]),
    NEAR_AT(0x0b16, &g_gun_draw_steps[3]),
    NEAR_AT(0x0b25, &g_gun_draw_steps[4]),
    NEAR_AT(0x0b34, &g_gun_draw_steps[5]),
    NEAR_AT(0x0b43, &g_gun_draw_steps[6]),
};

struct point16 g_gun_form_sizes[7] = {   /* DGROUP 0x0b60 */
    { 64, 31 },
    { 56, 36 },
    { 128, 37 },
    { 112, 34 },
    { 128, 34 },
    { 128, 43 },
    { 64, 31 },
};

struct offset8 g_gun_hot_spots[7] = {   /* DGROUP 0x0b7c */
    { 0 },
    { -1, -5 },
    { -2, -3 },
    { -15, -6 },
    { -11, -3 },
    { -2, -12 },
    { 0 },
};

struct draw_step g_light_draw_steps[4] = {   /* DGROUP 0x0b8a */
    {
        0,    /* next */
        0x02,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 20, 28 } }    /* offset */
    },
    {
        0,    /* next */
        0x02,    /* level */
        { 0x01, 0x05, 0xff, 0xff },    /* frame */
        { { -8, -18 }, { 20, 28 } }    /* offset */
    },
    {
        0,    /* next */
        0x02,    /* level */
        { 0x02, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 19, 2 } }    /* offset */
    },
    {
        0,    /* next */
        0x02,    /* level */
        { 0x03, 0x05, 0xff, 0xff },    /* frame */
        { { -8, 0 }, { 19, 2 } }    /* offset */
    },
};

struct draw_step *g_light_form_steps[4] = {   /* DGROUP 0x0bc6 */
    NEAR_AT(0x0b8a, &g_light_draw_steps[0]),
    NEAR_AT(0x0b99, &g_light_draw_steps[1]),
    NEAR_AT(0x0ba8, &g_light_draw_steps[2]),
    NEAR_AT(0x0bb7, &g_light_draw_steps[3]),
};

struct point16 g_light_form_sizes[4] = {   /* DGROUP 0x0bce */
    { 32, 54 },
    { 47, 72 },
    { 32, 38 },
    { 47, 50 },
};

struct offset8 g_light_hot_spots[4] = {   /* DGROUP 0x0bde */
    { 0 },
    { -8, -18 },
    { 0 },
    { -8, 0 },
};

struct draw_step g_monkey_draw_steps[13] = {   /* DGROUP 0x0be6 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0, 12 }, { 40, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0xff, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x01, 0x05, 0xff, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0x05, 0xff, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x03, 0x05, 0xff, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x06, 0xff },    /* frame */
        { { 0, 12 }, { 40, 0 }, { 14, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x07, 0xff },    /* frame */
        { { 0, 12 }, { 40, 0 }, { 16, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x08, 0xff },    /* frame */
        { { 0, 12 }, { 40, 0 }, { 16, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x09, 0xff },    /* frame */
        { { 0, 12 }, { 40, 0 }, { 15, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x06, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 }, { 14, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x07, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 }, { 16, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x08, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 }, { 16, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x09, 0xff },    /* frame */
        { { 0, 12 }, { 40, 6 }, { 14, 1 } }    /* offset */
    },
};

struct draw_step *g_monkey_form_steps[13] = {   /* DGROUP 0x0ca9 */
    NEAR_AT(0x0be6, &g_monkey_draw_steps[0]),
    NEAR_AT(0x0bf5, &g_monkey_draw_steps[1]),
    NEAR_AT(0x0c04, &g_monkey_draw_steps[2]),
    NEAR_AT(0x0c13, &g_monkey_draw_steps[3]),
    NEAR_AT(0x0c22, &g_monkey_draw_steps[4]),
    NEAR_AT(0x0c31, &g_monkey_draw_steps[5]),
    NEAR_AT(0x0c40, &g_monkey_draw_steps[6]),
    NEAR_AT(0x0c4f, &g_monkey_draw_steps[7]),
    NEAR_AT(0x0c5e, &g_monkey_draw_steps[8]),
    NEAR_AT(0x0c6d, &g_monkey_draw_steps[9]),
    NEAR_AT(0x0c7c, &g_monkey_draw_steps[10]),
    NEAR_AT(0x0c8b, &g_monkey_draw_steps[11]),
    NEAR_AT(0x0c9a, &g_monkey_draw_steps[12]),
};

struct point16 g_monkey_form_sizes[13] = {   /* DGROUP 0x0cc3 */
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
    { 92, 79 },
};

struct draw_step g_rocket_draw_steps[10] = {   /* DGROUP 0x0cf7 */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 3, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0 }, { 2, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x03, 0xff, 0xff },    /* frame */
        { { 0 }, { 2, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 0, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x05, 0xff, 0xff },    /* frame */
        { { 0 }, { -2, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x06, 0xff, 0xff },    /* frame */
        { { 0 }, { -2, 45 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x07, 0xff, 0xff },    /* frame */
        { { 0 }, { 0, 44 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x08, 0xff, 0xff },    /* frame */
        { { 0 }, { 1, 46 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x09, 0xff, 0xff },    /* frame */
        { { 0 }, { 0, 46 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0a, 0xff, 0xff },    /* frame */
        { { 0 }, { 0, 46 } }    /* offset */
    },
};

struct draw_step *g_rocket_form_steps[10] = {   /* DGROUP 0x0d8d */
    NEAR_AT(0x0cf7, &g_rocket_draw_steps[0]),
    NEAR_AT(0x0d06, &g_rocket_draw_steps[1]),
    NEAR_AT(0x0d15, &g_rocket_draw_steps[2]),
    NEAR_AT(0x0d24, &g_rocket_draw_steps[3]),
    NEAR_AT(0x0d33, &g_rocket_draw_steps[4]),
    NEAR_AT(0x0d42, &g_rocket_draw_steps[5]),
    NEAR_AT(0x0d51, &g_rocket_draw_steps[6]),
    NEAR_AT(0x0d60, &g_rocket_draw_steps[7]),
    NEAR_AT(0x0d6f, &g_rocket_draw_steps[8]),
    NEAR_AT(0x0d7e, &g_rocket_draw_steps[9]),
};

struct point16 g_rocket_form_sizes[10] = {   /* DGROUP 0x0da1 */
    { 16, 66 },
    { 16, 74 },
    { 16, 70 },
    { 16, 65 },
    { 18, 59 },
    { 18, 56 },
    { 16, 66 },
    { 16, 81 },
    { 16, 83 },
    { 16, 82 },
};

struct offset8 g_rocket_hot_spots[10] = {   /* DGROUP 0x0dc9 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { -2, 0 },
    { -2, 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
};

struct draw_step g_scissors_draw_steps[3] = {   /* DGROUP 0x0ddd */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { 21, 17 } }    /* offset */
    },
    {
        NEAR_AT(0x0ddd, &g_scissors_draw_steps[0]),    /* next */
        0x00,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { -2, 4 } }    /* offset */
    },
};

struct draw_step *g_scissors_form_steps[2] = {   /* DGROUP 0x0e0a */
    NEAR_AT(0x0dec, &g_scissors_draw_steps[1]),
    NEAR_AT(0x0dfb, &g_scissors_draw_steps[2]),
};

struct point16 g_scissors_form_sizes[2] = {   /* DGROUP 0x0e0e */
    { 40, 34 },
    { 48, 24 },
};

struct offset8 g_scissors_hot_spots[2] = {   /* DGROUP 0x0e16 */
    { 0 },
    { -2, 0 },
};

struct draw_step g_solar_panel_draw_steps[4] = {   /* DGROUP 0x0e1a */
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 52, 4 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 52, 18 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x01, 0xff },    /* frame */
        { { 0 }, { 52, 4 }, { 52, 18 } }    /* offset */
    },
};

struct draw_step *g_solar_panel_form_steps[4] = {   /* DGROUP 0x0e56 */
    NEAR_AT(0x0e1a, &g_solar_panel_draw_steps[0]),
    NEAR_AT(0x0e29, &g_solar_panel_draw_steps[1]),
    NEAR_AT(0x0e38, &g_solar_panel_draw_steps[2]),
    NEAR_AT(0x0e47, &g_solar_panel_draw_steps[3]),
};

struct point16 g_solar_panel_form_sizes[4] = {   /* DGROUP 0x0e5e */
    { 72, 32 },
    { 72, 32 },
    { 72, 32 },
    { 72, 32 },
};

struct draw_step g_trampoline_draw_steps[10] = {   /* DGROUP 0x0e6e */
    {
        0,    /* next */
        0x00,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { 0, 9 } }    /* offset */
    },
    {
        NEAR_AT(0x0e6e, &g_trampoline_draw_steps[0]),    /* next */
        0x04,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x03, 0xff, 0xff, 0xff },    /* frame */
        { { 0, 9 } }    /* offset */
    },
    {
        NEAR_AT(0x0e8c, &g_trampoline_draw_steps[2]),    /* next */
        0x04,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x05, 0xff, 0xff, 0xff },    /* frame */
        { { 0, 9 } }    /* offset */
    },
    {
        NEAR_AT(0x0eaa, &g_trampoline_draw_steps[4]),    /* next */
        0x04,    /* level */
        { 0x04, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { 0, 9 } }    /* offset */
    },
    {
        NEAR_AT(0x0ec8, &g_trampoline_draw_steps[6]),    /* next */
        0x04,    /* level */
        { 0x06, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x00,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { 0, 9 } }    /* offset */
    },
    {
        NEAR_AT(0x0ee6, &g_trampoline_draw_steps[8]),    /* next */
        0x04,    /* level */
        { 0x07, 0xff, 0xff, 0xff },    /* frame */
        { { 0, -3 } }    /* offset */
    },
};

struct draw_step *g_trampoline_form_steps[5] = {   /* DGROUP 0x0f04 */
    NEAR_AT(0x0e7d, &g_trampoline_draw_steps[1]),
    NEAR_AT(0x0e9b, &g_trampoline_draw_steps[3]),
    NEAR_AT(0x0eb9, &g_trampoline_draw_steps[5]),
    NEAR_AT(0x0ed7, &g_trampoline_draw_steps[7]),
    NEAR_AT(0x0ef5, &g_trampoline_draw_steps[9]),
};

struct point16 g_trampoline_form_sizes[5] = {   /* DGROUP 0x0f0e */
    { 48, 28 },
    { 48, 28 },
    { 48, 28 },
    { 48, 28 },
    { 48, 31 },
};

struct offset8 g_trampoline_hot_spots[5] = {   /* DGROUP 0x0f22 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, -3 },
};

struct draw_step g_candle_draw_steps[6] = {   /* DGROUP 0x0f2c */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 11, -4 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x02, 0xff, 0xff },    /* frame */
        { { 0 }, { 11, -4 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x03, 0xff, 0xff },    /* frame */
        { { 0 }, { 11, -4 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x04, 0xff, 0xff },    /* frame */
        { { 0 }, { 11, -4 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x05, 0xff, 0xff },    /* frame */
        { { 0 }, { 11, -4 } }    /* offset */
    },
};

struct draw_step *g_candle_form_steps[6] = {   /* DGROUP 0x0f86 */
    NEAR_AT(0x0f2c, &g_candle_draw_steps[0]),
    NEAR_AT(0x0f3b, &g_candle_draw_steps[1]),
    NEAR_AT(0x0f4a, &g_candle_draw_steps[2]),
    NEAR_AT(0x0f59, &g_candle_draw_steps[3]),
    NEAR_AT(0x0f68, &g_candle_draw_steps[4]),
    NEAR_AT(0x0f77, &g_candle_draw_steps[5]),
};

struct point16 g_candle_form_sizes[6] = {   /* DGROUP 0x0f92 */
    { 34, 32 },
    { 34, 36 },
    { 34, 36 },
    { 34, 36 },
    { 34, 36 },
    { 34, 36 },
};

struct offset8 g_candle_hot_spots[6] = {   /* DGROUP 0x0faa */
    { 0 },
    { 0, -4 },
    { 0, -4 },
    { 0, -4 },
    { 0, -4 },
    { 0, -4 },
};

struct draw_step g_kind_51_draw_steps[9] = {   /* DGROUP 0x0fb6 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x03, 0x00, 0x05, 0xff },    /* frame */
        { { 0 }, { 17, 32 }, { 7, 9 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x06, 0x03, 0x04, 0x01 },    /* frame */
        { { 3, 7 }, { 0, -4 }, { 44, 31 }, { 18, 28 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x07, 0x03, 0x00, 0x04 },    /* frame */
        { { 0, 9 }, { 0 }, { 17, 32 }, { 47, 35 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x08, 0x03, 0x02, 0x04 },    /* frame */
        { { -3, 11 }, { 0, 1 }, { 15, 34 }, { 49, 37 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x09, 0x03, 0x00, 0x04 },    /* frame */
        { { -3, 8 }, { 0 }, { 17, 32 }, { 47, 35 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x09, 0x03, 0x04, 0x01 },    /* frame */
        { { -2, 5 }, { 0, -4 }, { 44, 31 }, { 18, 28 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x09, 0x03, 0x00, 0x04 },    /* frame */
        { { -3, 8 }, { 0 }, { 17, 32 }, { 47, 35 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x09, 0x03, 0x02, 0x04 },    /* frame */
        { { -3, 9 }, { 0, 1 }, { 15, 34 }, { 49, 37 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x09, 0x03, 0x00, 0x04 },    /* frame */
        { { -3, 8 }, { 0 }, { 17, 32 }, { 47, 35 } }    /* offset */
    },
};

struct draw_step *g_kind_51_form_steps[9] = {   /* DGROUP 0x103d */
    NEAR_AT(0x0fb6, &g_kind_51_draw_steps[0]),
    NEAR_AT(0x0fc5, &g_kind_51_draw_steps[1]),
    NEAR_AT(0x0fd4, &g_kind_51_draw_steps[2]),
    NEAR_AT(0x0fe3, &g_kind_51_draw_steps[3]),
    NEAR_AT(0x0ff2, &g_kind_51_draw_steps[4]),
    NEAR_AT(0x1001, &g_kind_51_draw_steps[5]),
    NEAR_AT(0x1010, &g_kind_51_draw_steps[6]),
    NEAR_AT(0x101f, &g_kind_51_draw_steps[7]),
    NEAR_AT(0x102e, &g_kind_51_draw_steps[8]),
};

struct point16 g_kind_51_form_sizes[9] = {   /* DGROUP 0x104f */
    { 50, 50 },
    { 58, 51 },
    { 62, 50 },
    { 66, 50 },
    { 64, 50 },
    { 64, 51 },
    { 64, 50 },
    { 66, 50 },
    { 64, 50 },
};

struct offset8 g_kind_51_hot_spots[9] = {   /* DGROUP 0x1073 */
    { 0 },
    { 0, -4 },
    { 0 },
    { -3, 1 },
    { -3, 0 },
    { -2, -4 },
    { -3, 0 },
    { -3, 1 },
    { -3, 0 },
};

struct draw_step g_kind_58_draw_steps[9] = {   /* DGROUP 0x1085 */
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0xff, 0xff },    /* frame */
        { { 0 }, { 28, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x02, 0xff },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x04, 0xff },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0x06 },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 }, { -12, -28 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0x07 },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 }, { -17, -30 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0x08 },    /* frame */
        { { 0 }, { 28, 39 }, { 8, 39 }, { -18, -33 } }    /* offset */
    },
};

struct draw_step *g_kind_58_form_steps[9] = {   /* DGROUP 0x110c */
    NEAR_AT(0x1085, &g_kind_58_draw_steps[0]),
    NEAR_AT(0x1094, &g_kind_58_draw_steps[1]),
    NEAR_AT(0x10a3, &g_kind_58_draw_steps[2]),
    NEAR_AT(0x10b2, &g_kind_58_draw_steps[3]),
    NEAR_AT(0x10c1, &g_kind_58_draw_steps[4]),
    NEAR_AT(0x10d0, &g_kind_58_draw_steps[5]),
    NEAR_AT(0x10df, &g_kind_58_draw_steps[6]),
    NEAR_AT(0x10ee, &g_kind_58_draw_steps[7]),
    NEAR_AT(0x10fd, &g_kind_58_draw_steps[8]),
};

struct point16 g_kind_58_form_sizes[9] = {   /* DGROUP 0x111e */
    { 48, 64 },
    { 48, 64 },
    { 48, 64 },
    { 48, 64 },
    { 48, 64 },
    { 48, 64 },
    { 60, 92 },
    { 65, 94 },
    { 66, 97 },
};

struct offset8 g_kind_58_hot_spots[9] = {   /* DGROUP 0x1142 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { -12, -28 },
    { -17, -30 },
    { -18, -33 },
};

struct draw_step g_kind_61_draw_steps[15] = {   /* DGROUP 0x1154 */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0b, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 2 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0c, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 2 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0d, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 2 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0e, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 1 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0f, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 2 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x01, 0x0a, 0x09 },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 }, { 21, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 2, -10 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x03, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 4, -17 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 2, -10 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x06, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x07, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x08, 0x0a, 0xff },    /* frame */
        { { 37, 0 }, { 0 }, { 62, 0 } }    /* offset */
    },
};

struct draw_step *g_kind_61_form_steps[15] = {   /* DGROUP 0x1235 */
    NEAR_AT(0x1154, &g_kind_61_draw_steps[0]),
    NEAR_AT(0x1163, &g_kind_61_draw_steps[1]),
    NEAR_AT(0x1172, &g_kind_61_draw_steps[2]),
    NEAR_AT(0x1181, &g_kind_61_draw_steps[3]),
    NEAR_AT(0x1190, &g_kind_61_draw_steps[4]),
    NEAR_AT(0x119f, &g_kind_61_draw_steps[5]),
    NEAR_AT(0x11ae, &g_kind_61_draw_steps[6]),
    NEAR_AT(0x11bd, &g_kind_61_draw_steps[7]),
    NEAR_AT(0x11cc, &g_kind_61_draw_steps[8]),
    NEAR_AT(0x11db, &g_kind_61_draw_steps[9]),
    NEAR_AT(0x11ea, &g_kind_61_draw_steps[10]),
    NEAR_AT(0x11f9, &g_kind_61_draw_steps[11]),
    NEAR_AT(0x1208, &g_kind_61_draw_steps[12]),
    NEAR_AT(0x1217, &g_kind_61_draw_steps[13]),
    NEAR_AT(0x1226, &g_kind_61_draw_steps[14]),
};

struct point16 g_kind_61_form_sizes[15] = {   /* DGROUP 0x1253 */
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 26 },
    { 80, 33 },
    { 80, 26 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
    { 80, 16 },
};

struct offset8 g_kind_61_hot_spots[15] = {   /* DGROUP 0x128f */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, -10 },
    { 0, -17 },
    { 0, -10 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
};

struct draw_step g_kind_62_draw_steps[12] = {   /* DGROUP 0x12ad */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0xff, 0xff, 0xff },    /* frame */
        { { 0 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x01, 0xff, 0xff, 0xff },    /* frame */
        { { 2, -4 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { 3, -13 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { 3, -13 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { 3, -13 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x02, 0xff, 0xff, 0xff },    /* frame */
        { { 3, -13 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x04, 0x06, 0xff, 0xff },    /* frame */
        { { -5, 0 }, { -46, -34 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x05, 0x07, 0xff, 0xff },    /* frame */
        { { -6, 0 }, { -42, -34 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x04, 0x08, 0xff, 0xff },    /* frame */
        { { -5, 0 }, { -45, -35 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x05, 0x06, 0xff, 0xff },    /* frame */
        { { -6, 0 }, { -45, -34 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x04, 0x07, 0xff, 0xff },    /* frame */
        { { -5, 0 }, { -44, -34 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x05, 0x08, 0xff, 0xff },    /* frame */
        { { -6, 0 }, { -44, -35 } }    /* offset */
    },
};

struct draw_step *g_kind_62_form_steps[12] = {   /* DGROUP 0x1361 */
    NEAR_AT(0x12ad, &g_kind_62_draw_steps[0]),
    NEAR_AT(0x12bc, &g_kind_62_draw_steps[1]),
    NEAR_AT(0x12cb, &g_kind_62_draw_steps[2]),
    NEAR_AT(0x12da, &g_kind_62_draw_steps[3]),
    NEAR_AT(0x12e9, &g_kind_62_draw_steps[4]),
    NEAR_AT(0x12f8, &g_kind_62_draw_steps[5]),
    NEAR_AT(0x1307, &g_kind_62_draw_steps[6]),
    NEAR_AT(0x1316, &g_kind_62_draw_steps[7]),
    NEAR_AT(0x1325, &g_kind_62_draw_steps[8]),
    NEAR_AT(0x1334, &g_kind_62_draw_steps[9]),
    NEAR_AT(0x1343, &g_kind_62_draw_steps[10]),
    NEAR_AT(0x1352, &g_kind_62_draw_steps[11]),
};

struct point16 g_kind_62_form_sizes[12] = {   /* DGROUP 0x1379 */
    { 30, 22 },
    { 36, 26 },
    { 42, 35 },
    { 42, 35 },
    { 42, 35 },
    { 42, 35 },
    { 82, 56 },
    { 78, 56 },
    { 81, 57 },
    { 81, 56 },
    { 80, 56 },
    { 80, 57 },
};

struct offset8 g_kind_62_hot_spots[12] = {   /* DGROUP 0x13a9 */
    { 0 },
    { 0, -4 },
    { 0, -13 },
    { 0, -13 },
    { 0, -13 },
    { 0, -13 },
    { -46, -34 },
    { -42, -34 },
    { -45, -35 },
    { -45, -34 },
    { -44, -34 },
    { -44, -35 },
};

struct offset8 g_seesaw_hot_spots[3] = {   /* DGROUP 0x13c1 */
    { 0 },
    { 0, 12 },
    { 0 },
};

struct offset8 g_balloon_hot_spots[7] = {   /* DGROUP 0x13c7 */
    { 0 },
    { -15, -9 },
    { -20, -5 },
    { -28, 8 },
    { -30, 25 },
    { -26, 41 },
    { -26, 56 },
};

struct offset8 g_pokey_hot_spots[10] = {   /* DGROUP 0x13d5 */
    { 0 },
    { -6, -16 },
    { 19, -1 },
    { 14, 0 },
    { 10, 0 },
    { 8, 0 },
    { 6, -2 },
    { -1, -2 },
    { -5, -2 },
    { -9, -3 },
};

struct offset8 g_bellow_hot_spots[3] = {   /* DGROUP 0x13e9 */
    { 0 },
    { -8, 8 },
    { -11, 12 },
};

struct point16 g_bellow_form_sizes[3] = {   /* DGROUP 0x13ef */
    { 64, 48 },
    { 72, 48 },
    { 80, 48 },
};

struct offset8 g_bullet_hot_spots[3] = {   /* DGROUP 0x13fb */
    { 0 },
    { 4, -3 },
    { 1, -12 },
};

struct offset8 g_flashlight_hot_spots[2] = {   /* DGROUP 0x1401 */
    { 0 },
    { 0, -10 },
};

struct offset8 g_boxing_glove_hot_spots[10] = {   /* DGROUP 0x1405 */
    { 0 },
    { 7, -12 },
    { -29, -3 },
    { -84, -6 },
    { -15, -3 },
    { -29, -3 },
    { -30, 5 },
    { -21, 5 },
    { -25, 6 },
    { -21, 6 },
};

struct offset8 g_windmill_hot_spots[4] = {   /* DGROUP 0x1419 */
    { 0 },
    { -3, -3 },
    { -4, -4 },
    { -3, -3 },
};

struct offset8 g_blast_hot_spots[6] = {   /* DGROUP 0x1421 */
    { 0 },
    { -4, -13 },
    { 0, -6 },
    { 13, 9 },
    { 20, 19 },
    { 20, 20 },
};

struct offset8 g_mort_the_mouse_hot_spots[2] = {   /* DGROUP 0x142d */
    { 0 },
    { 0, 1 },
};

struct offset8 g_kind_54_hot_spots[31] = {   /* DGROUP 0x1431 */
    { 0 },
    { 0, -1 },
    { 0, -1 },
    { 0 },
    { 0 },
    { 0, -1 },
    { -2, -6 },
    { -8, -6 },
    { -7, -6 },
    { -7, -3 },
    { -10, 14 },
    { -14, 17 },
    { -8, 9 },
    { -3, 3 },
    { -1, 1 },
    { -1, 0 },
    { 1, 0 },
    { -2, 0 },
    { 1, 0 },
    { -1, 0 },
    { -3, 0 },
    { -2, 0 },
    { -1, 0 },
    { 1, -7 },
    { -1, -7 },
    { 1, -6 },
    { -1, -6 },
    { -2, 0 },
    { -8, 2 },
    { -8, 9 },
    { -11, 16 },
};

struct point16 g_kind_54_form_sizes[31] = {   /* DGROUP 0x146f */
    { 16, 24 },
    { 16, 24 },
    { 16, 24 },
    { 16, 24 },
    { 16, 24 },
    { 16, 24 },
    { 24, 30 },
    { 32, 31 },
    { 24, 30 },
    { 32, 26 },
    { 40, 24 },
    { 48, 24 },
    { 32, 24 },
    { 24, 24 },
    { 16, 24 },
    { 16, 24 },
    { 16, 24 },
    { 24, 24 },
    { 24, 24 },
    { 16, 24 },
    { 24, 24 },
    { 24, 24 },
    { 16, 24 },
    { 24, 31 },
    { 24, 31 },
    { 24, 30 },
    { 16, 30 },
    { 24, 24 },
    { 24, 24 },
    { 24, 24 },
    { 24, 24 },
};


/*
 * **The part kinds**, 66 records of 0x3a bytes, in 1.11 in a far data
 * segment of their own - image 0x2ef10, before DGROUP - where 1.00 kept 58
 * in DGROUP at 0x0ea6. See `g_part_kinds` in dgroup.h for how the game
 * reaches a record. Written by `tools/kindtables.py --kinds` from the image.
 *
 * **The hooks are far pointers the loader relocates**, six a record. The
 * routine each names is the port's transcription at that address; the new
 * kinds' own routines are stubs until they are transcribed.
 *
 * `bitmaps` is 0 for every kind here: `load_part_bitmap` fills it at run
 * time. The field names and the kinds' names are ours - see `struct
 * part_kind`.
 */
struct part_kind far g_part_kinds[PART_KIND_COUNT] = {
    {   /* 0 bowling_ball */
        0x0b10,    /* density */
        0x00c8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0000,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_big_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 1 brick_platform */
        0x1039,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0018,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        240,    /* max_w */
        240,    /* max_h */
        16,    /* min_w */
        16,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0028,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_platform,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_settle_platform,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 2 ramp */
        0x05e6,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        64,    /* max_w */
        0,    /* max_h */
        16,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x002c,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_ramp,    /* setup */
        part_flip_ramp,    /* flip */
        part_settle_ramp,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 3 seesaw */
        0x0760,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x13c1, g_seesaw_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0006,    /* priority */
        part_hit_seesaw,    /* hit */
        part_step_seesaw,    /* step */
        part_setup_seesaw,    /* setup */
        part_flip_seesaw,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_seesaw)    /* drive */
    },
    {   /* 4 balloon */
        0x0009,    /* density */
        0x0001,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x13c7, g_balloon_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0005,    /* priority */
        part_hit_balloon,    /* hit */
        part_step_balloon,    /* step */
        part_setup_balloon,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_balloon)    /* drive */
    },
    {   /* 5 conveyor */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        96,    /* max_w */
        0,    /* max_h */
        32,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x000c,    /* priority */
        part_hit_conveyor,    /* hit */
        part_step_conveyor,    /* step */
        part_setup_conveyor,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_settle_conveyor,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 6 mouse_cage */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0025,    /* priority */
        part_hit_mouse_cage,    /* hit */
        part_step_mouse_cage,    /* step */
        part_setup_mouse_cage,    /* setup */
        part_flip_mouse_cage,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 7 pulley */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0011,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 8 belt */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x01, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x000a,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 9 basketball */
        0x052a,    /* density */
        0x0014,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0001,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_big_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 10 rope */
        0x0640,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x01, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x000f,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 11 bird_cage */
        0x1d80,    /* density */
        0x0096,    /* weight */
        0x0020,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x000c,    /* point_count */
        0x0022,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_bird_cage,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_bird_cage)    /* drive */
    },
    {   /* 12 pokey */
        0x07d0,    /* density */
        0x0078,    /* weight */
        0x0000,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x13d5, g_pokey_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0023,    /* priority */
        part_hit_pokey,    /* hit */
        part_step_pokey,    /* step */
        part_setup_pokey,    /* setup */
        part_flip_pokey,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 13 jack_in_the_box */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x03c4, g_jack_in_the_box_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0436, g_jack_in_the_box_hot_spots),    /* hotspots */
        NEAR_AT(0x03ea, g_jack_in_the_box_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x000d,    /* priority */
        part_hook_yes,    /* hit */
        part_step_jack_in_the_box,    /* step */
        part_setup_jack_in_the_box,    /* setup */
        part_flip_jack_in_the_box,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 14 gear */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0030,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x000b,    /* priority */
        part_hit_gear,    /* hit */
        part_step_gear,    /* step */
        part_setup_gear,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 15 bob_the_fish */
        0x07d0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x05b5, g_bob_the_fish_form_steps),    /* bitmaps2 */
        NEAR_AT(0x063f, g_bob_the_fish_hot_spots),    /* hotspots */
        NEAR_AT(0x05e3, g_bob_the_fish_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0026,    /* priority */
        part_hit_bob_the_fish,    /* hit */
        part_step_bob_the_fish,    /* step */
        part_setup_bob_the_fish,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 16 bellow */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x13e9, g_bellow_hot_spots),    /* hotspots */
        NEAR_AT(0x13ef, g_bellow_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x0007,    /* priority */
        part_hit_bellow,    /* hit */
        part_step_bellow,    /* step */
        part_setup_bellow,    /* setup */
        part_flip_bellow,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 17 bucket */
        0x1d80,    /* density */
        0x0064,    /* weight */
        0x0020,    /* bounce */
        0x0030,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x0021,    /* priority */
        part_hit_bucket,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_bucket,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_bucket)    /* drive */
    },
    {   /* 18 cannon */
        0x3986,    /* density */
        0x03e8,    /* weight */
        0x00c0,    /* bounce */
        0x000c,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x074e, g_cannon_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0796, g_cannon_hot_spots),    /* hotspots */
        NEAR_AT(0x0766, g_cannon_form_sizes),    /* sizes */
        { 0x04, 0x00 },    /* refile_level */
        0x0008,    /* point_count */
        0x001c,    /* priority */
        part_hook_yes,    /* hit */
        part_step_cannon,    /* step */
        part_setup_cannon,    /* setup */
        part_flip_cannon,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 19 dynamite */
        0x046c,    /* density */
        0x005a,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0808, g_dynamite_form_steps),    /* bitmaps2 */
        NEAR_AT(0x082c, g_dynamite_hot_spots),    /* hotspots */
        NEAR_AT(0x0814, g_dynamite_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x001d,    /* priority */
        part_hit_dynamite,    /* hit */
        part_step_dynamite,    /* step */
        part_setup_dynamite,    /* setup */
        part_flip_dynamite,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 20 bullet */
        0x0000,    /* density */
        0x4e20,    /* weight */
        0x0000,    /* bounce */
        0x0001,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x13fb, g_bullet_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0032,    /* priority */
        part_hit_bullet,    /* hit */
        part_step_bullet,    /* step */
        part_setup_bullet,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 21 electric_plug */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x08b0, g_electric_plug_form_steps),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x08c0, g_electric_plug_form_sizes),    /* sizes */
        { 0x05, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0014,    /* priority */
        part_hit_electric_plug,    /* hit */
        part_step_electric_plug,    /* step */
        part_setup_electric_plug,    /* setup */
        part_flip_electric_plug,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 22 dynamite_plunger */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x090d, g_dynamite_plunger_form_steps),    /* bitmaps2 */
        NEAR_AT(0x091f, g_dynamite_plunger_hot_spots),    /* hotspots */
        NEAR_AT(0x0913, g_dynamite_plunger_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0020,    /* priority */
        part_hit_dynamite_plunger,    /* hit */
        part_step_dynamite_plunger,    /* step */
        part_setup_dynamite_plunger,    /* setup */
        part_flip_dynamite_plunger,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_dynamite_plunger)    /* drive */
    },
    {   /* 23 hook */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0010,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_hook,    /* setup */
        part_flip_hook,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 24 fan */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0961, g_fan_form_steps),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x0969, g_fan_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0017,    /* priority */
        part_hook_yes,    /* hit */
        part_step_fan,    /* step */
        part_setup_fan,    /* setup */
        part_flip_fan,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 25 flashlight */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x1401, g_flashlight_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x001a,    /* priority */
        part_hit_flashlight,    /* hit */
        part_step_flashlight,    /* step */
        part_setup_flashlight,    /* setup */
        part_flip_flashlight,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 26 generator */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0a69, g_generator_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0ac9, g_generator_hot_spots),    /* hotspots */
        NEAR_AT(0x0a89, g_generator_form_sizes),    /* sizes */
        { 0x05, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0015,    /* priority */
        part_hit_generator,    /* hit */
        part_step_generator,    /* step */
        part_setup_generator,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 27 gun */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0b52, g_gun_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0b7c, g_gun_hot_spots),    /* hotspots */
        NEAR_AT(0x0b60, g_gun_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x0012,    /* priority */
        part_hook_yes,    /* hit */
        part_step_gun,    /* step */
        part_setup_gun,    /* setup */
        part_flip_gun,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_gun)    /* drive */
    },
    {   /* 28 baseball */
        0x07d0,    /* density */
        0x0009,    /* weight */
        0x0040,    /* bounce */
        0x0018,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0003,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_small_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 29 light */
        0x0514,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0bc6, g_light_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0bde, g_light_hot_spots),    /* hotspots */
        NEAR_AT(0x0bce, g_light_form_sizes),    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x001b,    /* priority */
        part_hit_light,    /* hit */
        part_step_light,    /* step */
        part_setup_light,    /* setup */
        part_flip_light,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_light)    /* drive */
    },
    {   /* 30 magnifying_glass */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0019,    /* priority */
        part_hook_yes,    /* hit */
        part_step_magnifying_glass,    /* step */
        part_setup_magnifying_glass,    /* setup */
        part_flip_magnifying_glass,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 31 monkey */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0ca9, g_monkey_form_steps),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x0cc3, g_monkey_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0009,    /* point_count */
        0x0027,    /* priority */
        part_hit_monkey,    /* hit */
        part_step_monkey,    /* step */
        part_setup_monkey,    /* setup */
        part_flip_monkey,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_monkey)    /* drive */
    },
    {   /* 32 pumpkin */
        0x0960,    /* density */
        0x0064,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x003d,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_pumpkin,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 33 heart_balloon */
        0x000b,    /* density */
        0x0004,    /* weight */
        0x0080,    /* bounce */
        0x0008,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x003e,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_heart_balloon,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_heart_balloon)    /* drive */
    },
    {   /* 34 christmas_tree */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x003f,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_christmas_tree,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 35 boxing_glove */
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x1405, g_boxing_glove_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x0008,    /* priority */
        part_hit_boxing_glove,    /* hit */
        part_step_boxing_glove,    /* step */
        part_setup_boxing_glove,    /* setup */
        part_flip_boxing_glove,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 36 rocket */
        0x4650,    /* density */
        0x0708,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0d8d, g_rocket_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0dc9, g_rocket_hot_spots),    /* hotspots */
        NEAR_AT(0x0da1, g_rocket_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x001e,    /* priority */
        part_hook_yes,    /* hit */
        part_step_rocket,    /* step */
        part_setup_rocket,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 37 scissors */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0e0a, g_scissors_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0e16, g_scissors_hot_spots),    /* hotspots */
        NEAR_AT(0x0e0e, g_scissors_form_sizes),    /* sizes */
        { 0x04, 0x00 },    /* refile_level */
        0x0008,    /* point_count */
        0x0013,    /* priority */
        part_hit_scissors,    /* hit */
        part_step_scissors,    /* step */
        part_setup_scissors,    /* setup */
        part_flip_scissors,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 38 solar_panel */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0e56, g_solar_panel_form_steps),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x0e5e, g_solar_panel_form_sizes),    /* sizes */
        { 0x05, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0016,    /* priority */
        part_hook_yes,    /* hit */
        part_step_solar_panel,    /* step */
        part_setup_solar_panel,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 39 trampoline */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0f04, g_trampoline_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0f22, g_trampoline_hot_spots),    /* hotspots */
        NEAR_AT(0x0f0e, g_trampoline_form_sizes),    /* sizes */
        { 0x04, 0x00 },    /* refile_level */
        0x0004,    /* point_count */
        0x0009,    /* priority */
        part_hit_trampoline,    /* hit */
        part_step_trampoline,    /* step */
        part_setup_trampoline,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 40 windmill */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x1419, g_windmill_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0003,    /* point_count */
        0x000e,    /* priority */
        part_hook_yes,    /* hit */
        part_step_windmill,    /* step */
        part_setup_windmill,    /* setup */
        part_flip_windmill,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 41 blast */
        0x0000,    /* density */
        0x0001,    /* weight */
        0x0100,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x1421, g_blast_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x00, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_step_blast,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 42 mort_the_mouse */
        0x07d0,    /* density */
        0x0001,    /* weight */
        0x0000,    /* bounce */
        0x0100,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x142d, g_mort_the_mouse_hot_spots),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0024,    /* priority */
        part_hit_mort_the_mouse,    /* hit */
        part_step_mort_the_mouse,    /* step */
        part_setup_mort_the_mouse,    /* setup */
        part_flip_mort_the_mouse,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 43 cannon_ball */
        0x53b4,    /* density */
        0x6d60,    /* weight */
        0x0020,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0002,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_cannon_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 44 tennis_ball */
        0x052a,    /* density */
        0x0005,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0004,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_small_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 45 candle */
        0x07d0,    /* density */
        0x000c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0f86, g_candle_form_steps),    /* bitmaps2 */
        NEAR_AT(0x0faa, g_candle_hot_spots),    /* hotspots */
        NEAR_AT(0x0f92, g_candle_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0003,    /* point_count */
        0x001f,    /* priority */
        part_hook_yes,    /* hit */
        part_step_candle,    /* step */
        part_setup_candle,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 46 pipe */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        240,    /* max_w */
        240,    /* max_h */
        32,    /* min_w */
        32,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0029,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_platform,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_settle_platform,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 47 corner_pipe */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x002a,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_corner_pipe,    /* setup */
        (void (far *)())part_flip_corner_pipe,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 48 wooden_platform */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x000c,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        240,    /* max_w */
        240,    /* max_h */
        32,    /* min_w */
        32,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x002b,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_platform,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_settle_platform,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 49 anchor */
        0x0640,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 50 motor */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0018,    /* priority */
        part_hook_yes,    /* hit */
        part_step_motor,    /* step */
        part_setup_motor,    /* setup */
        part_flip_motor,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 51 kind_51 */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x103d, g_kind_51_form_steps),    /* bitmaps2 */
        NEAR_AT(0x1073, g_kind_51_hot_spots),    /* hotspots */
        NEAR_AT(0x104f, g_kind_51_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0032,    /* priority */
        part_hit_kind_51,    /* hit */
        part_step_kind_51,    /* step */
        part_setup_kind_51,    /* setup */
        part_flip_kind_51,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 52 kind_52 */
        0x05dc,    /* density */
        0x0028,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x0033,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_kind_52,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 53 kind_53 */
        0x1d80,    /* density */
        0x0014,    /* weight */
        0x0400,    /* bounce */
        0x0002,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0003,    /* point_count */
        0x0034,    /* priority */
        part_hit_kind_53,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_kind_53,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 54 kind_54 */
        0x0960,    /* density */
        0x0028,    /* weight */
        0x0080,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x1431, g_kind_54_hot_spots),    /* hotspots */
        NEAR_AT(0x146f, g_kind_54_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0035,    /* priority */
        part_hit_kind_54,    /* hit */
        part_step_kind_54,    /* step */
        part_setup_kind_54,    /* setup */
        part_flip_kind_54,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 55 kind_55 */
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_kinds_55_57,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 56 kind_56 */
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x05, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_kind_56,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 57 kind_57 */
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x00, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_step_kind_57,    /* step */
        part_setup_kinds_55_57,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_drive_kind_57)    /* drive */
    },
    {   /* 58 kind_58 */
        0x1d80,    /* density */
        0x0fa0,    /* weight */
        0x0400,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x110c, g_kind_58_form_steps),    /* bitmaps2 */
        NEAR_AT(0x1142, g_kind_58_hot_spots),    /* hotspots */
        NEAR_AT(0x111e, g_kind_58_form_sizes),    /* sizes */
        { 0x05, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x0036,    /* priority */
        part_hook_yes,    /* hit */
        part_step_kind_58,    /* step */
        part_hook_none_2a6,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 59 kind_59 */
        0x0708,    /* density */
        0x000e,    /* weight */
        0x0200,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0037,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_cannon_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 60 kind_60 */
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0400,    /* bounce */
        0x0080,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        240,    /* max_w */
        240,    /* max_h */
        16,    /* min_w */
        16,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0038,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_platform,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_settle_platform,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 61 kind_61 */
        0x0960,    /* density */
        0x0028,    /* weight */
        0x0400,    /* bounce */
        0x0030,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x1235, g_kind_61_form_steps),    /* bitmaps2 */
        NEAR_AT(0x128f, g_kind_61_hot_spots),    /* hotspots */
        NEAR_AT(0x1253, g_kind_61_form_sizes),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0039,    /* priority */
        part_hit_kind_61,    /* hit */
        part_step_kind_61,    /* step */
        part_setup_kind_61,    /* setup */
        part_flip_kind_61,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 62 kind_62 */
        0x0960,    /* density */
        0x0028,    /* weight */
        0x0040,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x1361, g_kind_62_form_steps),    /* bitmaps2 */
        NEAR_AT(0x13a9, g_kind_62_hot_spots),    /* hotspots */
        NEAR_AT(0x1379, g_kind_62_form_sizes),    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0006,    /* point_count */
        0x003a,    /* priority */
        part_hook_yes,    /* hit */
        part_step_kind_62,    /* step */
        part_setup_kind_62,    /* setup */
        part_flip_kind_62,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 63 kind_63 */
        0x0960,    /* density */
        0x0028,    /* weight */
        0x0100,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x003b,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_cannon_ball,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 64 kind_64 */
        0x0960,    /* density */
        0x0028,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x003c,    /* priority */
        part_hit_kind_64,    /* hit */
        part_step_kind_64,    /* step */
        part_setup_kind_64,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {   /* 65 kind_65 */
        0x07d0,    /* density */
        0x0320,    /* weight */
        0x0080,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0,    /* max_w */
        0,    /* max_h */
        240,    /* min_w */
        240,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0005,    /* point_count */
        0x0041,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_kind_65,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
};

/*
 * DGROUP 0x4e4e..0x53fc - **this module's `_BSS`**: the game's state that
 * starts out zero. It is the first of the game's, right after the run-time
 * library's initialised data, and `collide.c`'s, the next object's, begins
 * at 0x53fc. **Its order is the reverse of the definitions below**, which
 * is how Borland C++ 3.0 lays out `_BSS` - by first mention, last first
 * (docs/lessons.md) - and so dgroup.h's `extern`s for these, which would be
 * their first mention, are hidden from this file (`GAMEDATA_C`).
 *
 * `_stklen` is the start-up's stack length, which the program defines
 * itself - so TLINK takes none from the library.
 */
struct game_directories g_game_directories;   /* DGROUP 0x4f14 */
char g_picked_machine[0xd];   /* DGROUP 0x4f07 */
uint16_t _stklen;   /* DGROUP 0x4f05 */
/* DGROUP 0x4f01 in 1.11, new there: **the memory free at start-up**, the
   largest DOS block over 1000 - what `g_preload_sound_need` is measured in.
   The name is a guess. */
int32_t  g_memory_k;
uint16_t g_stop_requested;   /* DGROUP 0x4eff  game_teardown(0) raises it; the loops above read it */
FILE     *g_tim_sx;   /* DGROUP 0x4efd  tim.sx's file record, which open_sound_file reads the sounds from */
struct bitmap **g_cursor_art;   /* DGROUP 0x4efb  mouse.bmp's list */
struct bitmap **g_panel_art;   /* DGROUP 0x4ef9  the art set the panel's pieces come out of */
uint16_t g_cursor_follows;   /* DGROUP 0x4ef7  restore_cursor_following is guarded by this */
/* DGROUP 0x4ef5 in 1.11 (1.00: 0x52f1, a byte): the last key the screen
   loops took, as `translate_key` answers it - scancode high, character low.
   The loops test the character, `(g_last_key & 0x7f)`. */
uint16_t g_last_key;
/*
 * DGROUP 0x52ed  tim.pal: the far pointer `load_palette` answers, stored whole and
 * read whole by `set_palette_pointer` and `free_far_block`.
 */
uint8_t far *g_pal_tim;
uint8_t far *g_pal_dynamix;   /* DGROUP 0x4eed  sierra.pal */
uint8_t far *g_pal_black;   /* DGROUP 0x4ee9  black.pal, as pal_tim */
/* **The font handle for "memofnt8.fnt"**, what `load_font` answered at
   start-up; `set_font` takes it and `game_teardown` gives its slot back. */
int16_t g_memo_font;   /* DGROUP 0x4ee7 */
/* **The saved clip rectangle**, stored in descending order - 0x52dd is the
   left edge and 0x52d7 the bottom, which looks like a transcription error
   and is not. */
int16_t g_saved_clip_left;   /* DGROUP 0x4ee5 */
int16_t g_saved_clip_right;   /* DGROUP 0x4ee3 */
int16_t g_saved_clip_top;   /* DGROUP 0x4ee1 */
int16_t g_saved_clip_bottom;   /* DGROUP 0x4edf */
int16_t g_music_now;   /* DGROUP 0x4edd  the tune opened and started, remembered */
/* **Four request-and-acknowledge words**, one per machine sound: something
   sets one to 2, and `run_machine_loop`, which does all four every frame,
   turns it to 1 and then stops the sound. */
int16_t g_sound_request_01;   /* DGROUP 0x4edb */
int16_t g_sound_request_02;   /* DGROUP 0x4ed9 */
int16_t g_sound_request_09;   /* DGROUP 0x4ed7 */
int16_t g_sound_request_0c;   /* DGROUP 0x4ed5 */
/* 1.11 adds two, at DGROUP 0x4ed3 and 0x4ed1 (0x4ed5 is 1.00's 0x52cd): for
   sound 0x15, which kind 51 holds while it runs, and 0x19, kind 62's. */
int16_t g_sound_request_15;
int16_t g_sound_request_19;
int16_t g_fill_colour;   /* DGROUP 0x4ecf  the colour the panel and the title box are filled in */
/* **The colour the parts bin's column is cleared to**, 0x0b, filed once by
   `game_setup` and read only by `draw_machine_layer_a`, which puts it in
   both of the driver's fill colours before its two `fill_rect`s. */
int16_t g_bin_colour;   /* DGROUP 0x4ecd */
int16_t g_drop_cursor;   /* DGROUP 0x4ecb  0xa on every frame the hand is not already carrying */
int16_t g_band_colour;   /* DGROUP 0x4ec9  0xa where it would attach, -1 for no line */
int16_t g_anchor_y;   /* DGROUP 0x4ec7 */
int16_t g_anchor_x;   /* DGROUP 0x4ec5  the far part's anchor: its +0x1e and +0x20 plus +0x56, +0x57 */
int16_t g_band_y;   /* DGROUP 0x4ec3 */
int16_t g_band_x;   /* DGROUP 0x4ec1  the pointer in play-area coordinates */
struct part g_placed_parts;   /* DGROUP 0x4e1f */
struct part g_moving_parts;   /* DGROUP 0x4d7d */
struct held_parts g_held_parts;   /* DGROUP 0x4cd7 */
/* DGROUP 0x50cb..0x50d3: nothing in the image names these eight bytes. Ours. */
uint8_t g_dg50cb[8];
struct part *g_layer_head[6];   /* DGROUP 0x4cc3 */
struct level_settings g_level_settings;   /* DGROUP 0x4cb3 */
/* **The level's title and hint**, read from the level file by
   `load_level` when it is a level and written back by `write_level`;
   the briefing draws the title over the panel and wraps the hint into
   the box. Eighty bytes for the title is the distance to the hint; the
   hint's extent is the gap to the next record at 0x50af, and the reader
   (`game_fread_string`, a length byte then the bytes) can put at most
   255 in it. */
char g_level_hint[0x190];   /* DGROUP 0x4b23, up to g_level_settings */
char g_level_title[0x50];   /* DGROUP 0x4ad3 */
struct bitmap **g_score2_bmp;   /* DGROUP 0x4ad1  score2.bmp's - draw_odometer_digit's strips */
struct bitmap **g_border_art;   /* DGROUP 0x4acf  gp_bord.bmp's */
struct bitmap **g_menu_bmp;   /* DGROUP 0x4acd  gp_menu.bmp's */
struct bitmap **g_icons_bmp;   /* DGROUP 0x4acb  icons.bmp's list */
int16_t g_cursor;   /* DGROUP 0x4ac9 */
/* **The cursor showing, and the one the hourglass replaced.**
   `select_cursor` returns at once when the number it is given is already
   in `cursor`, `wait_cursor` files the outgoing one in `g_saved_cursor`
   unless it is the hourglass itself, and `restore_cursor` selects what is
   there. */
int16_t g_saved_cursor;   /* DGROUP 0x4ac7 */
int16_t g_master_level;   /* DGROUP 0x4ac5  the volume knob's setting; in tim.cfg */
int16_t g_playing;   /* DGROUP 0x4ac3  game_play runs while this is non-zero */
int16_t g_round_number;   /* DGROUP 0x4ac1  the puzzle being played; round_setup loads it */
/* **Written once and never read**, and that is the whole of what is known:
   `round_setup` stores 0 here, and the two bytes of this offset occur
   exactly once in the image - that store, in 1.00 and 1.11 alike. A dead
   store of the original's. The name says what is measured. */
uint16_t g_round_unread;   /* DGROUP 0x4abf */
int16_t g_level_count;   /* DGROUP 0x4abd  how many g_l<n>.LEV there are */
int16_t g_furthest_level;   /* DGROUP 0x4abb  how far the player has reached; in tim.cfg */
int16_t g_password_puzzle;   /* DGROUP 0x4ab9  the puzzle game_teardown prints a password for */
/* **How far each bonus counter has rolled**, 0 to 0x15 - one digit cell -
   and back to 0 with one off the counter's value. `start_counters` puts
   the first at -4, which is four steps of nothing before it moves, and
   `step_counters` draws the band only while the scroll is positive. The
   second reel's is never armed in the shipped game; see `step_counters`
   and STATUS.md. */
int16_t g_bonus_1_scroll;   /* DGROUP 0x4ab7 */
int16_t g_bonus_2_scroll;   /* DGROUP 0x4ab5 */
int32_t g_odometer_total;   /* DGROUP 0x4ab1  the odometer's running total */
/* **Two 32-bit scores.** `finish_level` copies `g_odometer_total` into
   `g_banked_score` a word at a time, and each is read as one `int32_t`, by
   `score_to_code` and by the odometer. */
int32_t g_banked_score;   /* DGROUP 0x4aad  what finish_level banks for the password */
/* `run_machine_loop` accumulates the ticks a frame took in `g_elapsed_ticks`
   and counts its frames in `g_machine_frames`. */
uint16_t g_machine_frames;   /* DGROUP 0x4aab */
uint16_t g_elapsed_ticks;   /* DGROUP 0x4aa9 */
/* **Three origin pairs**, y then x, all set to -8 by `round_setup`; which
   is which role is not established, only that the live one is the third,
   `g_origin_y`/`g_origin_x`: the play area's scroll origin, which
   `draw_part_clip` takes from world coordinates to get the screen's. */
int16_t g_origin_x;   /* DGROUP 0x4aa7 */
int16_t g_origin_y;   /* DGROUP 0x4aa5 */
int16_t g_origin_b_x;   /* DGROUP 0x4aa3 */
int16_t g_origin_b_y;   /* DGROUP 0x4aa1 */
int16_t g_origin_c_x;   /* DGROUP 0x4a9f */
int16_t g_origin_c_y;   /* DGROUP 0x4a9d */
uint16_t g_drag_offset_x;   /* DGROUP 0x4a9b */
/* **Where in the part the player took hold of it**: the pointer less the
   part's own origin, filed when a part is picked up and subtracted again
   every frame, so a part grabbed by its corner stays held by its corner.
   The y is first, which is the order the original writes them in. */
uint16_t g_drag_offset_y;   /* DGROUP 0x4a99 */
/* **Five deferred redraws**, one layer each: a change asks for N frames and
   gets one a frame. Counts, not flags - `game_screen_loop` decrements each
   by one rather than clearing it. */
uint16_t g_redraw_e;   /* DGROUP 0x4a97 */
uint16_t g_redraw_d;   /* DGROUP 0x4a95 */
uint16_t g_redraw_c;   /* DGROUP 0x4a93 */
uint16_t g_redraw_b;   /* DGROUP 0x4a91 */
uint16_t g_redraw_a;   /* DGROUP 0x4a8f */
/* **A countdown for the carried part's icon**, the same shape as a part's
   own `redraw_count`: the editor loop draws the icon and steps it down
   while it is not zero. */
uint16_t g_redraw_carried;   /* DGROUP 0x4a8d */
/* **Frames the loop that is running has run.** `step_loop_frames` adds one
   a frame from the intro's loop and from `run_machine_loop`, `round_setup`
   clears it, and `draw_machine_layer_f` clears it on its way in - which is
   what freezes the bin's header animation at frame 0, the one thing that
   reads it as a phase. The other reader is `goal_test_puzzle_70`, which
   wants 0x134 of them before it will pass, so on that puzzle it is
   elapsed time. Not `g_machine_frames`: that one `clear_machine` resets at
   every start and this one only a new round does. */
int16_t g_loop_frames;   /* DGROUP 0x4a8b  wraps 0x2a00 to 0x1c00 */
uint16_t g_file_op_active;   /* DGROUP 0x4a89  GUESS: 1 around the chdir a file dialog does */
/* **The "memory is getting low" box has been shown.** Set with the box and
   cleared again only when the largest free block climbs back over 0x1770,
   which is the hysteresis that stops a machine hovering near the edge
   being told twice. */
uint16_t g_memory_warned;   /* DGROUP 0x4a87 */
int16_t g_holiday_valentine;   /* DGROUP 0x4a85 (1.00: 0x4e81)  14 February - kind 33, the heart */
int16_t g_holiday_stpatrick;   /* DGROUP 0x4a83 (1.00: 0x4e7f)  17 March    - kind 65 in 1.11 */
int16_t g_holiday_halloween;   /* DGROUP 0x4a81 (1.00: 0x4e7d)  31 October  - kind 32, the pumpkin */
int16_t g_holiday_christmas;   /* DGROUP 0x4a7f (1.00: 0x4e7b)  25 December - kind 34, the tree */
int16_t g_holiday_july4;       /* DGROUP 0x4a7d, new in 1.11   4 July      - read by nothing */
struct region *g_regions_play;   /* DGROUP 0x4a7b  the play screen's */
struct region *g_regions_panel;   /* DGROUP 0x4a79  the briefing's controls */
struct region *g_regions_c;   /* DGROUP 0x4a77 */
struct region *g_regions_b;   /* DGROUP 0x4a75 */
struct region *g_regions_a;   /* DGROUP 0x4a73  the five region lists, heads of */
struct region *g_region_kept_b;   /* DGROUP 0x4a71 */
struct region *g_region_kept_a;   /* DGROUP 0x4a6f  two records kept on their own as well */
uint16_t g_round_state;   /* DGROUP 0x4a6d  the round and screen state machine's word */
/* **Which handle the pointer is on**, and the only word that says what a
   click in the play area will do. 0 is nothing, 1 to 8 are the handles
   `part_handle_at_pointer` answers - the two flips, the four resize
   corners and the two ends - and 9 is "carrying a part". Bit 0x8000 says
   the handle is engaged, so `g_tool & 0x7fff` is the handle and the bit is
   the drag. `cursor_for_tool` turns the nine into cursor numbers, which is
   where the name comes from. */
uint16_t g_tool;   /* DGROUP 0x4a6b */
/* DGROUP 0x4a67 in 1.11, new there: **a machine file that carries its own
   parts bin** - read from a machine file of version 0x105 on, and what makes
   `read_level` read the bin list of a file that is not a level. 1.11 lets a
   machine's bin be set ("ADJUST PARTS BIN"). The name is ours. */
uint16_t g_freeform;   /* DGROUP 0x4a69  1 in freeform mode - the bin is unlimited and nothing is scored - 0 on a loaded level */
uint16_t g_machine_has_bin;   /* DGROUP 0x4a67 */
char g_picked_name[0xd];   /* DGROUP 0x4a5a */
struct queue_node *g_parts_queue;   /* DGROUP 0x4a58 */
struct queue_node *g_parts_free;   /* DGROUP 0x4a56 */
struct shape far *g_shapes_drawn;   /* DGROUP 0x4a52 */
struct shape far *g_shape_free;   /* DGROUP 0x4a4e */
