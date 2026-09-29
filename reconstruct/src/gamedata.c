/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game's tables: the part kinds, their drawing tables, and the
 * message strings** - DGROUP 0x0116..0x2370, data and no code - **and the
 * game state that starts out zero**, its `_BSS` 0x4e4e..0x53fc.
 *
 * Where it stands in the link is measured: TLINK lays each segment out in
 * object order, and this data comes after `gamemain.c`'s and before
 * `intro.c`'s. It is neither's. Borland C++ puts a module's string literals
 * after all its other initialised data, and `gamemain.c`'s `_DATA` is its
 * literals and nothing else, so what follows them was another object's; and
 * `intro.c`'s begins at 0x2370. **The file is ours**: that this was one
 * object and not several - the tables in one, the strings in another - is
 * not measured, as nothing in the range is padded to show where an object
 * ended. The objects came out of dgroup.c, their initialisers made
 * positional (tools/c89init.py) because Borland C++ has no designators, and
 * the two all-zero tables given `= { 0 }` so they stay in `_DATA`.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x0116..0x2370
 */
/* this file defines what dgroup.h declares `extern` for the rest: see the
   `_BSS` below */
#define GAMEDATA_C
/* Named before <dos.h>, which dgroup.h includes and which names `_stklen`:
   Borland lays `_BSS` out last mention first, and these two follow
   `_stklen` in the image. */
extern struct game_directories GAME_DIRECTORIES;
extern struct picked_machine PICKED_MACHINE;
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

struct game_master_levels GAME_MASTER_LEVELS = { { 0x0000, 0x0003, 0x0005, 0x0008, 0x000a, 0x000d, 0x000f } };

/* The draw step `draw_part` fills in for a part whose kind has no table. */
struct draw_step DEFAULT_DRAW_STEP = { 0, 0, { 0, 0xff }, { { 0 } } };   /* DGROUP 0x0124 */

/*
 * DGROUP 0x0133..0x0ea6 - **the kinds' drawing tables**, which `PART_KINDS`
 * points into and nothing else does. A kind that draws in steps has a run of
 * `draw_step`s, then three tables indexed by form: the first step
 * (`bitmaps2`), the size (`sizes`) and the hot spot (`hotspots`); a kind
 * that draws one bitmap per form has at most the last two. The boundaries are
 * where the kind records point, and they tile the range with nothing left
 * over. The `next` links and the table entries are near pointers, as the
 * image has them.
 */
struct draw_step JACK_IN_THE_BOX_DRAW_STEPS[19] = {
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x04, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x06, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x07, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x08, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x08, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x09, 0xff },    /* frame */
        { { 0, 0x03 }, { 0 }, { 0x09, 0x08 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0a, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0, 0xeb }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0b, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0x01, 0xde }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0c, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfa, 0xc5 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0d, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0e, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfa, 0xe2 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0f, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x10, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x11, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x12, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x13, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x14, 0x02, 0xff },    /* frame */
        { { 0, 0x03 }, { 0xfc, 0xe7 }, { 0x09, 0x09 } }    /* offset */
    }
};
struct draw_step *JACK_IN_THE_BOX_FORM_STEPS[19] = {   /* DGROUP 0x0250 */
    NEAR_AT(0x0133, &JACK_IN_THE_BOX_DRAW_STEPS[0]),
    NEAR_AT(0x0142, &JACK_IN_THE_BOX_DRAW_STEPS[1]),
    NEAR_AT(0x0151, &JACK_IN_THE_BOX_DRAW_STEPS[2]),
    NEAR_AT(0x0160, &JACK_IN_THE_BOX_DRAW_STEPS[3]),
    NEAR_AT(0x016f, &JACK_IN_THE_BOX_DRAW_STEPS[4]),
    NEAR_AT(0x017e, &JACK_IN_THE_BOX_DRAW_STEPS[5]),
    NEAR_AT(0x018d, &JACK_IN_THE_BOX_DRAW_STEPS[6]),
    NEAR_AT(0x019c, &JACK_IN_THE_BOX_DRAW_STEPS[7]),
    NEAR_AT(0x01ab, &JACK_IN_THE_BOX_DRAW_STEPS[8]),
    NEAR_AT(0x01ba, &JACK_IN_THE_BOX_DRAW_STEPS[9]),
    NEAR_AT(0x01c9, &JACK_IN_THE_BOX_DRAW_STEPS[10]),
    NEAR_AT(0x01d8, &JACK_IN_THE_BOX_DRAW_STEPS[11]),
    NEAR_AT(0x01e7, &JACK_IN_THE_BOX_DRAW_STEPS[12]),
    NEAR_AT(0x01f6, &JACK_IN_THE_BOX_DRAW_STEPS[13]),
    NEAR_AT(0x0205, &JACK_IN_THE_BOX_DRAW_STEPS[14]),
    NEAR_AT(0x0214, &JACK_IN_THE_BOX_DRAW_STEPS[15]),
    NEAR_AT(0x0223, &JACK_IN_THE_BOX_DRAW_STEPS[16]),
    NEAR_AT(0x0232, &JACK_IN_THE_BOX_DRAW_STEPS[17]),
    NEAR_AT(0x0241, &JACK_IN_THE_BOX_DRAW_STEPS[18])
};
struct point16 JACK_IN_THE_BOX_FORM_SIZES[19] = {   /* DGROUP 0x0276 */
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0023, 0x0035 },
    { 0x0023, 0x0042 },
    { 0x0027, 0x005b },
    { 0x0024, 0x0039 },
    { 0x0026, 0x003e },
    { 0x0025, 0x0039 },
    { 0x0029, 0x0039 },
    { 0x0028, 0x0039 },
    { 0x0024, 0x0039 },
    { 0x0024, 0x0039 },
    { 0x0024, 0x0039 }
};
struct point8 JACK_IN_THE_BOX_HOT_SPOTS[19] = {   /* DGROUP 0x02c2 */
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0, 0xeb },
    { 0, 0xde },
    { 0xfa, 0xc4 },
    { 0xfc, 0xe7 },
    { 0xfa, 0xe2 },
    { 0xfc, 0xe7 },
    { 0xfc, 0xe7 },
    { 0xfc, 0xe7 },
    { 0xfc, 0xe7 },
    { 0xfc, 0xe7 },
    { 0xfc, 0xe7 }
};
struct draw_step BOB_THE_FISH_DRAW_STEPS[23] = {   /* DGROUP 0x02e8 */
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x01, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x0a, 0x0b }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x02, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x0f, 0x09 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x03, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x1a, 0x10 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x04, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x1f, 0x11 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x05, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x1a, 0x10 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x06, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x0a, 0x0b }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x07, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x07, 0x0b }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x08, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x06, 0x0a }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x09, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x07, 0x10 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0a, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x08, 0x10 }, { 0x0a, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x00, 0x0b, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x06, 0x0a }, { 0x0a, 0x1b } }    /* offset */
    },
    { 0, 0x03, { 0x0d, 0xff, 0xff, 0xff }, { { 0xf0, 0x08 } } },
    { 0, 0x03, { 0x0e, 0xff, 0xff, 0xff }, { { 0xed, 0x0f } } },
    { 0, 0x03, { 0x0f, 0xff, 0xff, 0xff }, { { 0xe7, 0x13 } } },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x11, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x05, 0x27 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x12, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x05, 0x1f } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x13, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x03, 0x1d } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x14, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x05, 0x1d } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x15, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0xfb, 0x1b } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x16, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0xfb, 0x16 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x17, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x01, 0x1f } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x18, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x05, 0x24 } }    /* offset */
    },
    {
        0,    /* next */
        0x03,    /* level */
        { 0x10, 0x19, 0xff, 0xff },    /* frame */
        { { 0xe4, 0x19 }, { 0x05, 0x26 } }    /* offset */
    }
};
struct draw_step *BOB_THE_FISH_FORM_STEPS[23] = {   /* DGROUP 0x0441 */
    NEAR_AT(0x02e8, &BOB_THE_FISH_DRAW_STEPS[0]),
    NEAR_AT(0x02f7, &BOB_THE_FISH_DRAW_STEPS[1]),
    NEAR_AT(0x0306, &BOB_THE_FISH_DRAW_STEPS[2]),
    NEAR_AT(0x0315, &BOB_THE_FISH_DRAW_STEPS[3]),
    NEAR_AT(0x0324, &BOB_THE_FISH_DRAW_STEPS[4]),
    NEAR_AT(0x0333, &BOB_THE_FISH_DRAW_STEPS[5]),
    NEAR_AT(0x0342, &BOB_THE_FISH_DRAW_STEPS[6]),
    NEAR_AT(0x0351, &BOB_THE_FISH_DRAW_STEPS[7]),
    NEAR_AT(0x0360, &BOB_THE_FISH_DRAW_STEPS[8]),
    NEAR_AT(0x036f, &BOB_THE_FISH_DRAW_STEPS[9]),
    NEAR_AT(0x037e, &BOB_THE_FISH_DRAW_STEPS[10]),
    NEAR_AT(0x038d, &BOB_THE_FISH_DRAW_STEPS[11]),
    NEAR_AT(0x039c, &BOB_THE_FISH_DRAW_STEPS[12]),
    NEAR_AT(0x03ab, &BOB_THE_FISH_DRAW_STEPS[13]),
    NEAR_AT(0x03ba, &BOB_THE_FISH_DRAW_STEPS[14]),
    NEAR_AT(0x03c9, &BOB_THE_FISH_DRAW_STEPS[15]),
    NEAR_AT(0x03d8, &BOB_THE_FISH_DRAW_STEPS[16]),
    NEAR_AT(0x03e7, &BOB_THE_FISH_DRAW_STEPS[17]),
    NEAR_AT(0x03f6, &BOB_THE_FISH_DRAW_STEPS[18]),
    NEAR_AT(0x0405, &BOB_THE_FISH_DRAW_STEPS[19]),
    NEAR_AT(0x0414, &BOB_THE_FISH_DRAW_STEPS[20]),
    NEAR_AT(0x0423, &BOB_THE_FISH_DRAW_STEPS[21]),
    NEAR_AT(0x0432, &BOB_THE_FISH_DRAW_STEPS[22])
};
struct point16 BOB_THE_FISH_FORM_SIZES[23] = {   /* DGROUP 0x046f */
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0030, 0x0030 },
    { 0x0058, 0x002b },
    { 0x0058, 0x0026 },
    { 0x0060, 0x0025 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0023 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 },
    { 0x0068, 0x0020 }
};
struct point8 BOB_THE_FISH_HOT_SPOTS[23] = {   /* DGROUP 0x04cb */
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
    { 0xf0, 0x08 },
    { 0xed, 0x0f },
    { 0xe7, 0x13 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 },
    { 0xe4, 0x16 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 },
    { 0xe4, 0x19 }
};
struct draw_step CANNON_DRAW_STEPS[15] = {   /* DGROUP 0x04f9 */
    { 0, 0x04, { 0x00, 0x09, 0xff, 0xff }, { { 0 }, { 0x09, 0x0d } } },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0a, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0xf7, 0xfa } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0b, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0xf7, 0xfa } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0c, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0xfa, 0xfb } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0d, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0xf9, 0xfd } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0e, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0xfa, 0xfe } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x01, 0x09, 0xff, 0xff },    /* frame */
        { { 0xf9, 0xf8 }, { 0x09, 0x0d } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0x09, 0xff, 0xff },    /* frame */
        { { 0xf8, 0xf5 }, { 0x09, 0x0d } }    /* offset */
    },
    { 0, 0, { 0x04, 0xff, 0xff, 0xff }, { { 0x53, 0xdf } } },
    {
        NEAR_AT(0x0571, &CANNON_DRAW_STEPS[8]),    /* next */
        0x04,    /* level */
        { 0x03, 0x09, 0xff, 0xff },    /* frame */
        { { 0xfe, 0xfd }, { 0x09, 0x0d } }    /* offset */
    },
    { 0, 0, { 0x06, 0xff, 0xff, 0xff }, { { 0x65, 0xd2 } } },
    {
        NEAR_AT(0x058f, &CANNON_DRAW_STEPS[10]),    /* next */
        0x04,    /* level */
        { 0x05, 0x09, 0xff, 0xff },    /* frame */
        { { 0xfe, 0xf9 }, { 0x09, 0x0d } }    /* offset */
    },
    { 0, 0, { 0x08, 0xff, 0xff, 0xff }, { { 0x7f, 0xdf } } },
    {
        NEAR_AT(0x05ad, &CANNON_DRAW_STEPS[12]),    /* next */
        0x04,    /* level */
        { 0x07, 0x09, 0xff, 0xff },    /* frame */
        { { 0xfd, 0xfd }, { 0x09, 0x0d } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x09, 0x0f, 0xff },    /* frame */
        { { 0 }, { 0x09, 0x0d }, { 0, 0x02 } }    /* offset */
    }
};
struct draw_step *CANNON_FORM_STEPS[12] = {   /* DGROUP 0x05da */
    NEAR_AT(0x04f9, &CANNON_DRAW_STEPS[0]),
    NEAR_AT(0x0508, &CANNON_DRAW_STEPS[1]),
    NEAR_AT(0x0517, &CANNON_DRAW_STEPS[2]),
    NEAR_AT(0x0526, &CANNON_DRAW_STEPS[3]),
    NEAR_AT(0x0535, &CANNON_DRAW_STEPS[4]),
    NEAR_AT(0x0544, &CANNON_DRAW_STEPS[5]),
    NEAR_AT(0x0553, &CANNON_DRAW_STEPS[6]),
    NEAR_AT(0x0562, &CANNON_DRAW_STEPS[7]),
    NEAR_AT(0x0580, &CANNON_DRAW_STEPS[9]),
    NEAR_AT(0x059e, &CANNON_DRAW_STEPS[11]),
    NEAR_AT(0x05bc, &CANNON_DRAW_STEPS[13]),
    NEAR_AT(0x05cb, &CANNON_DRAW_STEPS[14])
};
struct point16 CANNON_FORM_SIZES[12] = {   /* DGROUP 0x05f2 */
    { 0x0040, 0x0034 },
    { 0x0049, 0x003a },
    { 0x0049, 0x003a },
    { 0x0046, 0x0039 },
    { 0x0047, 0x0037 },
    { 0x0046, 0x0036 },
    { 0x0035, 0x003c },
    { 0x003d, 0x003f },
    { 0x00c2, 0x0054 },
    { 0x00d2, 0x005e },
    { 0x00c7, 0x0054 },
    { 0x0040, 0x0034 }
};
struct point8 CANNON_HOT_SPOTS[12] = {   /* DGROUP 0x0622 */
    { 0 },
    { 0xf7, 0xfa },
    { 0xf7, 0xfa },
    { 0xfa, 0xfb },
    { 0xf9, 0xfd },
    { 0xfa, 0xfe },
    { 0xf9, 0xf8 },
    { 0xf8, 0xf5 },
    { 0xfe, 0xdf },
    { 0xfe, 0xd2 },
    { 0xfd, 0xdf }
};
struct draw_step DYNAMITE_DRAW_STEPS[6] = {   /* DGROUP 0x063a */
    { 0, 0x03, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0x03, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x28, 0x09 } } },
    { 0, 0x03, { 0x00, 0x02, 0xff, 0xff }, { { 0 }, { 0x27, 0x0a } } },
    { 0, 0x03, { 0x00, 0x03, 0xff, 0xff }, { { 0 }, { 0x27, 0x09 } } },
    { 0, 0x03, { 0x00, 0x04, 0xff, 0xff }, { { 0 }, { 0x25, 0x0d } } },
    { 0, 0x03, { 0x00, 0x05, 0xff, 0xff }, { { 0 }, { 0x26, 0x0c } } }
};
struct draw_step *DYNAMITE_FORM_STEPS[6] = { NEAR_AT(0x063a, &DYNAMITE_DRAW_STEPS[0]), NEAR_AT(0x0649, &DYNAMITE_DRAW_STEPS[1]), NEAR_AT(0x0658, &DYNAMITE_DRAW_STEPS[2]), NEAR_AT(0x0667, &DYNAMITE_DRAW_STEPS[3]), NEAR_AT(0x0676, &DYNAMITE_DRAW_STEPS[4]), NEAR_AT(0x0685, &DYNAMITE_DRAW_STEPS[5]) };   /* DGROUP 0x0694 */
struct point16 DYNAMITE_FORM_SIZES[6] = {   /* DGROUP 0x06a0 */
    { 0x0030, 0x001c },
    { 0x0038, 0x001c },
    { 0x0038, 0x001c },
    { 0x0038, 0x001c },
    { 0x0038, 0x001c },
    { 0x0038, 0x001c }
};
struct point8 DYNAMITE_HOT_SPOTS[6] = { 0 };   /* DGROUP 0x06b8 */
struct draw_step ELECTRIC_PLUG_DRAW_STEPS[8] = {   /* DGROUP 0x06c4 */
    { 0, 0x05, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x08, 0x08 } } },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0xff },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x03, 0x03 },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x04 }, { 0x1d, 0x12 } }    /* offset */
    },
    { 0, 0x05, { 0x00, 0x02, 0xff, 0xff }, { { 0 }, { 0x08, 0x08 } } },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0xff },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0xff },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x03, 0x03 },    /* frame */
        { { 0 }, { 0x08, 0x08 }, { 0x1d, 0x04 }, { 0x1d, 0x12 } }    /* offset */
    }
};
struct draw_step *ELECTRIC_PLUG_FORM_STEPS[8] = { NEAR_AT(0x06c4, &ELECTRIC_PLUG_DRAW_STEPS[0]), NEAR_AT(0x06d3, &ELECTRIC_PLUG_DRAW_STEPS[1]), NEAR_AT(0x06e2, &ELECTRIC_PLUG_DRAW_STEPS[2]), NEAR_AT(0x06f1, &ELECTRIC_PLUG_DRAW_STEPS[3]), NEAR_AT(0x0700, &ELECTRIC_PLUG_DRAW_STEPS[4]), NEAR_AT(0x070f, &ELECTRIC_PLUG_DRAW_STEPS[5]), NEAR_AT(0x071e, &ELECTRIC_PLUG_DRAW_STEPS[6]), NEAR_AT(0x072d, &ELECTRIC_PLUG_DRAW_STEPS[7]) };   /* DGROUP 0x073c */
struct point16 ELECTRIC_PLUG_FORM_SIZES[8] = {   /* DGROUP 0x074c */
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 },
    { 0x0030, 0x0020 }
};
struct draw_step DYNAMITE_PLUNGER_DRAW_STEPS[3] = {   /* DGROUP 0x076c */
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x01, 0xff },    /* frame */
        { { 0, 0x13 }, { 0x67, 0 }, { 0x28, 0x10 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x02, 0x01, 0xff },    /* frame */
        { { 0, 0x13 }, { 0x67, 0x05 }, { 0x28, 0x10 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x02, 0x01, 0xff, 0xff },    /* frame */
        { { 0x67, 0x0a }, { 0x28, 0x10 } }    /* offset */
    }
};
struct draw_step *DYNAMITE_PLUNGER_FORM_STEPS[3] = { NEAR_AT(0x076c, &DYNAMITE_PLUNGER_DRAW_STEPS[0]), NEAR_AT(0x077b, &DYNAMITE_PLUNGER_DRAW_STEPS[1]), NEAR_AT(0x078a, &DYNAMITE_PLUNGER_DRAW_STEPS[2]) };   /* DGROUP 0x0799 */
struct point16 DYNAMITE_PLUNGER_FORM_SIZES[3] = { { 0x0087, 0x0030 }, { 0x0087, 0x002e }, { 0x0087, 0x0029 } };   /* DGROUP 0x079f */
struct point8 DYNAMITE_PLUNGER_HOT_SPOTS[3] = { 0 };   /* DGROUP 0x07ab */
struct draw_step FAN_DRAW_STEPS[4] = {   /* DGROUP 0x07b1 */
    { 0, 0x04, { 0x00, 0x01, 0xff, 0xff }, { { 0, 0x08 }, { 0x10, 0 } } },
    { 0, 0x04, { 0x00, 0x02, 0xff, 0xff }, { { 0, 0x08 }, { 0x10, 0 } } },
    { 0, 0x04, { 0x00, 0x03, 0xff, 0xff }, { { 0, 0x08 }, { 0x10, 0 } } },
    { 0, 0x04, { 0x00, 0x04, 0xff, 0xff }, { { 0, 0x08 }, { 0x10, 0 } } }
};
struct draw_step *FAN_FORM_STEPS[4] = { NEAR_AT(0x07b1, &FAN_DRAW_STEPS[0]), NEAR_AT(0x07c0, &FAN_DRAW_STEPS[1]), NEAR_AT(0x07cf, &FAN_DRAW_STEPS[2]), NEAR_AT(0x07de, &FAN_DRAW_STEPS[3]) };   /* DGROUP 0x07ed */
struct point16 FAN_FORM_SIZES[4] = {   /* DGROUP 0x07f5 */
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 },
    { 0x0020, 0x0020 }
};
struct draw_step GENERATOR_DRAW_STEPS[16] = {   /* DGROUP 0x0805 */
    { 0, 0x05, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x15, 0 } } },
    { 0, 0x05, { 0x00, 0x02, 0xff, 0xff }, { { 0 }, { 0x15, 0xfa } } },
    { 0, 0x05, { 0x00, 0x03, 0xff, 0xff }, { { 0 }, { 0x15, 0x01 } } },
    { 0, 0x05, { 0x00, 0x04, 0xff, 0xff }, { { 0 }, { 0x15, 0xfb } } },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0 }, { 0x05, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0xfa }, { 0x05, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0x01 }, { 0x05, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0xfb }, { 0x05, 0x04 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0 }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0xfa }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0x01 }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0xff },    /* frame */
        { { 0 }, { 0x15, 0xfb }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x05, 0x05 },    /* frame */
        { { 0 }, { 0x15, 0 }, { 0x05, 0x04 }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x02, 0x05, 0x05 },    /* frame */
        { { 0 }, { 0x15, 0xfa }, { 0x05, 0x04 }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x03, 0x05, 0x05 },    /* frame */
        { { 0 }, { 0x15, 0x01 }, { 0x05, 0x04 }, { 0x05, 0x12 } }    /* offset */
    },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x04, 0x05, 0x05 },    /* frame */
        { { 0 }, { 0x15, 0xfb }, { 0x05, 0x04 }, { 0x05, 0x12 } }    /* offset */
    }
};
struct draw_step *GENERATOR_FORM_STEPS[16] = {   /* DGROUP 0x08f5 */
    NEAR_AT(0x0805, &GENERATOR_DRAW_STEPS[0]),
    NEAR_AT(0x0814, &GENERATOR_DRAW_STEPS[1]),
    NEAR_AT(0x0823, &GENERATOR_DRAW_STEPS[2]),
    NEAR_AT(0x0832, &GENERATOR_DRAW_STEPS[3]),
    NEAR_AT(0x0841, &GENERATOR_DRAW_STEPS[4]),
    NEAR_AT(0x0850, &GENERATOR_DRAW_STEPS[5]),
    NEAR_AT(0x085f, &GENERATOR_DRAW_STEPS[6]),
    NEAR_AT(0x086e, &GENERATOR_DRAW_STEPS[7]),
    NEAR_AT(0x087d, &GENERATOR_DRAW_STEPS[8]),
    NEAR_AT(0x088c, &GENERATOR_DRAW_STEPS[9]),
    NEAR_AT(0x089b, &GENERATOR_DRAW_STEPS[10]),
    NEAR_AT(0x08aa, &GENERATOR_DRAW_STEPS[11]),
    NEAR_AT(0x08b9, &GENERATOR_DRAW_STEPS[12]),
    NEAR_AT(0x08c8, &GENERATOR_DRAW_STEPS[13]),
    NEAR_AT(0x08d7, &GENERATOR_DRAW_STEPS[14]),
    NEAR_AT(0x08e6, &GENERATOR_DRAW_STEPS[15])
};
struct point16 GENERATOR_FORM_SIZES[16] = {   /* DGROUP 0x0915 */
    { 0x0050, 0x0020 },
    { 0x0050, 0x0026 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0025 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0026 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0025 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0026 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0025 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0026 },
    { 0x0050, 0x0020 },
    { 0x0050, 0x0025 }
};
struct point8 GENERATOR_HOT_SPOTS[16] = {   /* DGROUP 0x0955 */
    { 0 },
    { 0, 0xfa },
    { 0 },
    { 0, 0xfb },
    { 0 },
    { 0, 0xfa },
    { 0 },
    { 0, 0xfb },
    { 0 },
    { 0, 0xfa },
    { 0 },
    { 0, 0xfb },
    { 0 },
    { 0, 0xfa },
    { 0 },
    { 0, 0xfb }
};
struct draw_step GUN_DRAW_STEPS[7] = {   /* DGROUP 0x0975 */
    { 0, 0x04, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0x04, { 0x01, 0xff, 0xff, 0xff }, { { 0xff, 0xfb } } },
    { 0, 0x04, { 0x02, 0xff, 0xff, 0xff }, { { 0xfe, 0xfd } } },
    { 0, 0x04, { 0x03, 0xff, 0xff, 0xff }, { { 0xf1, 0xfa } } },
    { 0, 0x04, { 0x04, 0xff, 0xff, 0xff }, { { 0xf5, 0xfd } } },
    { 0, 0x04, { 0x00, 0x05, 0xff, 0xff }, { { 0xfe, 0 }, { 0x40, 0xf4 } } },
    { 0, 0x04, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } }
};
struct draw_step *GUN_FORM_STEPS[7] = { NEAR_AT(0x0975, &GUN_DRAW_STEPS[0]), NEAR_AT(0x0984, &GUN_DRAW_STEPS[1]), NEAR_AT(0x0993, &GUN_DRAW_STEPS[2]), NEAR_AT(0x09a2, &GUN_DRAW_STEPS[3]), NEAR_AT(0x09b1, &GUN_DRAW_STEPS[4]), NEAR_AT(0x09c0, &GUN_DRAW_STEPS[5]), NEAR_AT(0x09cf, &GUN_DRAW_STEPS[6]) };   /* DGROUP 0x09de */
struct point16 GUN_FORM_SIZES[7] = {   /* DGROUP 0x09ec */
    { 0x0040, 0x001f },
    { 0x0038, 0x0024 },
    { 0x0080, 0x0025 },
    { 0x0070, 0x0022 },
    { 0x0080, 0x0022 },
    { 0x0080, 0x002b },
    { 0x0040, 0x001f }
};
struct point8 GUN_HOT_SPOTS[7] = {   /* DGROUP 0x0a08 */
    { 0 },
    { 0xff, 0xfb },
    { 0xfe, 0xfd },
    { 0xf1, 0xfa },
    { 0xf5, 0xfd },
    { 0xfe, 0xf4 }
};
struct draw_step LIGHT_DRAW_STEPS[4] = {   /* DGROUP 0x0a16 */
    { 0, 0x02, { 0x00, 0x04, 0xff, 0xff }, { { 0 }, { 0x14, 0x1c } } },
    {
        0,    /* next */
        0x02,    /* level */
        { 0x01, 0x05, 0xff, 0xff },    /* frame */
        { { 0xf8, 0xee }, { 0x14, 0x1c } }    /* offset */
    },
    { 0, 0x02, { 0x02, 0x04, 0xff, 0xff }, { { 0 }, { 0x13, 0x02 } } },
    { 0, 0x02, { 0x03, 0x05, 0xff, 0xff }, { { 0xf8, 0 }, { 0x13, 0x02 } } }
};
struct draw_step *LIGHT_FORM_STEPS[4] = { NEAR_AT(0x0a16, &LIGHT_DRAW_STEPS[0]), NEAR_AT(0x0a25, &LIGHT_DRAW_STEPS[1]), NEAR_AT(0x0a34, &LIGHT_DRAW_STEPS[2]), NEAR_AT(0x0a43, &LIGHT_DRAW_STEPS[3]) };   /* DGROUP 0x0a52 */
struct point16 LIGHT_FORM_SIZES[4] = {   /* DGROUP 0x0a5a */
    { 0x0020, 0x0036 },
    { 0x002f, 0x0048 },
    { 0x0020, 0x0026 },
    { 0x002f, 0x0032 }
};
struct point8 LIGHT_HOT_SPOTS[4] = { { 0 }, { 0xf8, 0xee }, { 0 }, { 0xf8, 0 } };   /* DGROUP 0x0a6a */
struct draw_step MONKEY_DRAW_STEPS[13] = {   /* DGROUP 0x0a72 */
    { 0, 0x04, { 0x00, 0x04, 0xff, 0xff }, { { 0, 0x0c }, { 0x28, 0 } } },
    { 0, 0x04, { 0x00, 0x05, 0xff, 0xff }, { { 0, 0x0c }, { 0x28, 0x06 } } },
    { 0, 0x04, { 0x01, 0x05, 0xff, 0xff }, { { 0, 0x0c }, { 0x28, 0x06 } } },
    { 0, 0x04, { 0x02, 0x05, 0xff, 0xff }, { { 0, 0x0c }, { 0x28, 0x06 } } },
    { 0, 0x04, { 0x03, 0x05, 0xff, 0xff }, { { 0, 0x0c }, { 0x28, 0x06 } } },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x06, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0 }, { 0x0e, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x07, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0 }, { 0x10, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x08, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0 }, { 0x10, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x04, 0x09, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0 }, { 0x0f, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x06, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0x06 }, { 0x0e, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x07, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0x06 }, { 0x10, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x08, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0x06 }, { 0x10, 0x01 } }    /* offset */
    },
    {
        0,    /* next */
        0x04,    /* level */
        { 0x00, 0x05, 0x09, 0xff },    /* frame */
        { { 0, 0x0c }, { 0x28, 0x06 }, { 0x0e, 0x01 } }    /* offset */
    }
};
struct draw_step *MONKEY_FORM_STEPS[13] = {   /* DGROUP 0x0b35 */
    NEAR_AT(0x0a72, &MONKEY_DRAW_STEPS[0]),
    NEAR_AT(0x0a81, &MONKEY_DRAW_STEPS[1]),
    NEAR_AT(0x0a90, &MONKEY_DRAW_STEPS[2]),
    NEAR_AT(0x0a9f, &MONKEY_DRAW_STEPS[3]),
    NEAR_AT(0x0aae, &MONKEY_DRAW_STEPS[4]),
    NEAR_AT(0x0abd, &MONKEY_DRAW_STEPS[5]),
    NEAR_AT(0x0acc, &MONKEY_DRAW_STEPS[6]),
    NEAR_AT(0x0adb, &MONKEY_DRAW_STEPS[7]),
    NEAR_AT(0x0aea, &MONKEY_DRAW_STEPS[8]),
    NEAR_AT(0x0af9, &MONKEY_DRAW_STEPS[9]),
    NEAR_AT(0x0b08, &MONKEY_DRAW_STEPS[10]),
    NEAR_AT(0x0b17, &MONKEY_DRAW_STEPS[11]),
    NEAR_AT(0x0b26, &MONKEY_DRAW_STEPS[12])
};
struct point16 MONKEY_FORM_SIZES[13] = {   /* DGROUP 0x0b4f */
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f },
    { 0x005c, 0x004f }
};
struct draw_step ROCKET_DRAW_STEPS[10] = {   /* DGROUP 0x0b83 */
    { 0, 0x03, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x03, 0x2d } } },
    { 0, 0x03, { 0x00, 0x02, 0xff, 0xff }, { { 0 }, { 0x02, 0x2d } } },
    { 0, 0x03, { 0x00, 0x03, 0xff, 0xff }, { { 0 }, { 0x02, 0x2d } } },
    { 0, 0x03, { 0x00, 0x04, 0xff, 0xff }, { { 0 }, { 0, 0x2d } } },
    { 0, 0x03, { 0x00, 0x05, 0xff, 0xff }, { { 0 }, { 0xfe, 0x2d } } },
    { 0, 0x03, { 0x00, 0x06, 0xff, 0xff }, { { 0 }, { 0xfe, 0x2d } } },
    { 0, 0x03, { 0x00, 0x07, 0xff, 0xff }, { { 0 }, { 0, 0x2c } } },
    { 0, 0x03, { 0x00, 0x08, 0xff, 0xff }, { { 0 }, { 0x01, 0x2e } } },
    { 0, 0x03, { 0x00, 0x09, 0xff, 0xff }, { { 0 }, { 0, 0x2e } } },
    { 0, 0x03, { 0x00, 0x0a, 0xff, 0xff }, { { 0 }, { 0, 0x2e } } }
};
struct draw_step *ROCKET_FORM_STEPS[10] = {   /* DGROUP 0x0c19 */
    NEAR_AT(0x0b83, &ROCKET_DRAW_STEPS[0]),
    NEAR_AT(0x0b92, &ROCKET_DRAW_STEPS[1]),
    NEAR_AT(0x0ba1, &ROCKET_DRAW_STEPS[2]),
    NEAR_AT(0x0bb0, &ROCKET_DRAW_STEPS[3]),
    NEAR_AT(0x0bbf, &ROCKET_DRAW_STEPS[4]),
    NEAR_AT(0x0bce, &ROCKET_DRAW_STEPS[5]),
    NEAR_AT(0x0bdd, &ROCKET_DRAW_STEPS[6]),
    NEAR_AT(0x0bec, &ROCKET_DRAW_STEPS[7]),
    NEAR_AT(0x0bfb, &ROCKET_DRAW_STEPS[8]),
    NEAR_AT(0x0c0a, &ROCKET_DRAW_STEPS[9])
};
struct point16 ROCKET_FORM_SIZES[10] = {   /* DGROUP 0x0c2d */
    { 0x0010, 0x0042 },
    { 0x0010, 0x004a },
    { 0x0010, 0x0046 },
    { 0x0010, 0x0041 },
    { 0x0010, 0x003b },
    { 0x0010, 0x0038 },
    { 0x0010, 0x0042 },
    { 0x0010, 0x0051 },
    { 0x0010, 0x0053 },
    { 0x0010, 0x0052 }
};
struct point8 ROCKET_HOT_SPOTS[10] = { { 0 }, { 0 }, { 0 }, { 0 }, { 0xfe, 0 }, { 0xfe, 0 } };   /* DGROUP 0x0c55 */
struct draw_step SCISSORS_DRAW_STEPS[3] = {   /* DGROUP 0x0c69 */
    { 0, 0x04, { 0x01, 0xff, 0xff, 0xff }, { { 0x15, 0x11 } } },
    { NEAR_AT(0x0c69, &SCISSORS_DRAW_STEPS[0]), 0, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0x04, { 0x02, 0xff, 0xff, 0xff }, { { 0xfe, 0x04 } } }
};
struct draw_step *SCISSORS_FORM_STEPS[2] = { NEAR_AT(0x0c78, &SCISSORS_DRAW_STEPS[1]), NEAR_AT(0x0c87, &SCISSORS_DRAW_STEPS[2]) };   /* DGROUP 0x0c96 */
struct point16 SCISSORS_FORM_SIZES[2] = { { 0x0028, 0x0022 }, { 0x0030, 0x0018 } };   /* DGROUP 0x0c9a */
struct point8 SCISSORS_HOT_SPOTS[2] = { { 0 }, { 0xfe, 0 } };   /* DGROUP 0x0ca2 */
struct draw_step SOLAR_PANEL_DRAW_STEPS[4] = {   /* DGROUP 0x0ca6 */
    { 0, 0x05, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0x05, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x34, 0x04 } } },
    { 0, 0x05, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x34, 0x12 } } },
    {
        0,    /* next */
        0x05,    /* level */
        { 0x00, 0x01, 0x01, 0xff },    /* frame */
        { { 0 }, { 0x34, 0x04 }, { 0x34, 0x12 } }    /* offset */
    }
};
struct draw_step *SOLAR_PANEL_FORM_STEPS[4] = { NEAR_AT(0x0ca6, &SOLAR_PANEL_DRAW_STEPS[0]), NEAR_AT(0x0cb5, &SOLAR_PANEL_DRAW_STEPS[1]), NEAR_AT(0x0cc4, &SOLAR_PANEL_DRAW_STEPS[2]), NEAR_AT(0x0cd3, &SOLAR_PANEL_DRAW_STEPS[3]) };   /* DGROUP 0x0ce2 */
struct point16 SOLAR_PANEL_FORM_SIZES[4] = {   /* DGROUP 0x0cea */
    { 0x0048, 0x0020 },
    { 0x0048, 0x0020 },
    { 0x0048, 0x0020 },
    { 0x0048, 0x0020 }
};
struct draw_step TRAMPOLINE_DRAW_STEPS[10] = {   /* DGROUP 0x0cfa */
    { 0, 0, { 0x01, 0xff, 0xff, 0xff }, { { 0, 0x09 } } },
    { NEAR_AT(0x0cfa, &TRAMPOLINE_DRAW_STEPS[0]), 0x04, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0, { 0x03, 0xff, 0xff, 0xff }, { { 0, 0x09 } } },
    { NEAR_AT(0x0d18, &TRAMPOLINE_DRAW_STEPS[2]), 0x04, { 0x02, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0, { 0x05, 0xff, 0xff, 0xff }, { { 0, 0x09 } } },
    { NEAR_AT(0x0d36, &TRAMPOLINE_DRAW_STEPS[4]), 0x04, { 0x04, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0, { 0x01, 0xff, 0xff, 0xff }, { { 0, 0x09 } } },
    { NEAR_AT(0x0d54, &TRAMPOLINE_DRAW_STEPS[6]), 0x04, { 0x06, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0, { 0x01, 0xff, 0xff, 0xff }, { { 0, 0x09 } } },
    { NEAR_AT(0x0d72, &TRAMPOLINE_DRAW_STEPS[8]), 0x04, { 0x07, 0xff, 0xff, 0xff }, { { 0, 0xfd } } }
};
struct draw_step *TRAMPOLINE_FORM_STEPS[5] = { NEAR_AT(0x0d09, &TRAMPOLINE_DRAW_STEPS[1]), NEAR_AT(0x0d27, &TRAMPOLINE_DRAW_STEPS[3]), NEAR_AT(0x0d45, &TRAMPOLINE_DRAW_STEPS[5]), NEAR_AT(0x0d63, &TRAMPOLINE_DRAW_STEPS[7]), NEAR_AT(0x0d81, &TRAMPOLINE_DRAW_STEPS[9]) };   /* DGROUP 0x0d90 */
struct point16 TRAMPOLINE_FORM_SIZES[5] = {   /* DGROUP 0x0d9a */
    { 0x0030, 0x001c },
    { 0x0030, 0x001c },
    { 0x0030, 0x001c },
    { 0x0030, 0x001c },
    { 0x0030, 0x001f }
};
struct point8 TRAMPOLINE_HOT_SPOTS[5] = { { 0 }, { 0 }, { 0 }, { 0 }, { 0, 0xfd } };   /* DGROUP 0x0dae */
struct draw_step CANDLE_DRAW_STEPS[6] = {   /* DGROUP 0x0db8 */
    { 0, 0x03, { 0x00, 0xff, 0xff, 0xff }, { { 0 } } },
    { 0, 0x03, { 0x00, 0x01, 0xff, 0xff }, { { 0 }, { 0x0b, 0xfc } } },
    { 0, 0x03, { 0x00, 0x02, 0xff, 0xff }, { { 0 }, { 0x0b, 0xfc } } },
    { 0, 0x03, { 0x00, 0x03, 0xff, 0xff }, { { 0 }, { 0x0b, 0xfc } } },
    { 0, 0x03, { 0x00, 0x04, 0xff, 0xff }, { { 0 }, { 0x0b, 0xfc } } },
    { 0, 0x03, { 0x00, 0x05, 0xff, 0xff }, { { 0 }, { 0x0b, 0xfc } } }
};
struct draw_step *CANDLE_FORM_STEPS[6] = { NEAR_AT(0x0db8, &CANDLE_DRAW_STEPS[0]), NEAR_AT(0x0dc7, &CANDLE_DRAW_STEPS[1]), NEAR_AT(0x0dd6, &CANDLE_DRAW_STEPS[2]), NEAR_AT(0x0de5, &CANDLE_DRAW_STEPS[3]), NEAR_AT(0x0df4, &CANDLE_DRAW_STEPS[4]), NEAR_AT(0x0e03, &CANDLE_DRAW_STEPS[5]) };   /* DGROUP 0x0e12 */
struct point16 CANDLE_FORM_SIZES[6] = {   /* DGROUP 0x0e1e */
    { 0x0022, 0x0020 },
    { 0x0022, 0x0024 },
    { 0x0022, 0x0024 },
    { 0x0022, 0x0024 },
    { 0x0022, 0x0024 },
    { 0x0022, 0x0024 }
};
struct point8 CANDLE_HOT_SPOTS[6] = { { 0 }, { 0, 0xfc }, { 0, 0xfc }, { 0, 0xfc }, { 0, 0xfc }, { 0, 0xfc } };   /* DGROUP 0x0e36 */
struct point8 SEESAW_HOT_SPOTS[3] = { { 0 }, { 0, 0x0c } };   /* DGROUP 0x0e42 */
struct point8 BALLOON_HOT_SPOTS[7] = {   /* DGROUP 0x0e48 */
    { 0 },
    { 0xf1, 0xf7 },
    { 0xec, 0xfb },
    { 0xe4, 0x08 },
    { 0xe2, 0x19 },
    { 0xe6, 0x29 },
    { 0xe6, 0x38 }
};
struct point8 POKEY_HOT_SPOTS[10] = {   /* DGROUP 0x0e56 */
    { 0 },
    { 0xfa, 0xf0 },
    { 0x13, 0xff },
    { 0x0e, 0 },
    { 0x0a, 0 },
    { 0x08, 0 },
    { 0x06, 0xfe },
    { 0xff, 0xfe },
    { 0xfb, 0xfe },
    { 0xf7, 0xfd }
};
struct point8 BELLOW_HOT_SPOTS[3] = { { 0 }, { 0xf8, 0x08 }, { 0xf5, 0x0c } };   /* DGROUP 0x0e6a */
struct point8 BULLET_HOT_SPOTS[3] = { { 0 }, { 0x04, 0xfd }, { 0x01, 0xf4 } };   /* DGROUP 0x0e70 */
struct point8 FLASHLIGHT_HOT_SPOTS[2] = { { 0 }, { 0, 0xf6 } };   /* DGROUP 0x0e76 */
struct point8 BOXING_GLOVE_HOT_SPOTS[10] = {   /* DGROUP 0x0e7a */
    { 0 },
    { 0x07, 0xf4 },
    { 0xe3, 0xfd },
    { 0xac, 0xfa },
    { 0xf1, 0xfd },
    { 0xe3, 0xfd },
    { 0xe2, 0x05 },
    { 0xeb, 0x05 },
    { 0xe7, 0x06 },
    { 0xeb, 0x06 }
};
struct point8 WINDMILL_HOT_SPOTS[4] = { { 0 }, { 0xfd, 0xfd }, { 0xfc, 0xfc }, { 0xfd, 0xfd } };   /* DGROUP 0x0e8e */
struct point8 BLAST_HOT_SPOTS[6] = {   /* DGROUP 0x0e96 */
    { 0 },
    { 0xfc, 0xf3 },
    { 0, 0xfa },
    { 0x0d, 0x09 },
    { 0x14, 0x13 },
    { 0x14, 0x14 }
};
struct point8 MORT_THE_MOUSE_HOT_SPOTS[2] = { { 0 }, { 0, 0x01 } };   /* DGROUP 0x0ea2 */


/*
 * DGROUP 0x0ea6 - the part kinds, 58 records of 0x3a bytes, as the image holds
 * them. See `PART_KINDS` in dgroup.h for how the game reaches a record.
 *
 * **The hooks are far pointers the loader relocates** - segment 0000 for the
 * do-nothing routines at 0x0297..0x02b5, segment 172c for the rest - and they
 * are exactly the relocation table's entries in this range, six a record and
 * nothing else. So every `seg` is `LOAD_SEG +` the image's word, and no other
 * field is. The routine each names is the port's transcription at that address.
 *
 * `bitmaps` is 0 for every kind here: `load_part_bitmap` fills it at run
 * time. The field names and the kinds' names are ours - see `struct part_kind`.
 * The `word_*` fields are words nothing has been read for yet, transcribed as
 * they are.
 */
struct part_kind PART_KINDS[PART_KIND_COUNT] = {
    {
        0x0b10,    /* density */
        0x00c8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x1039,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0018,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0010,    /* min_w */
        0x0010,    /* min_h */
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
    {
        0x05e6,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0040,    /* max_w */
        0x0000,    /* max_h */
        0x0010,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0760,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e42, &SEESAW_HOT_SPOTS[0]),    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0006,    /* priority */
        part_hit_seesaw,    /* hit */
        part_step_seesaw,    /* step */
        part_setup_seesaw,    /* setup */
        part_flip_seesaw,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_seesaw    /* drive */
    },
    {
        0x0009,    /* density */
        0x0001,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e48, &BALLOON_HOT_SPOTS[0]),    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0005,    /* priority */
        part_hit_balloon,    /* hit */
        part_step_balloon,    /* step */
        part_setup_balloon,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_balloon    /* drive */
    },
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0060,    /* max_w */
        0x0000,    /* max_h */
        0x0020,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x052a,    /* density */
        0x0014,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0640,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x1d80,    /* density */
        0x0096,    /* weight */
        0x0020,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
        part_drive_bird_cage    /* drive */
    },
    {
        0x07d0,    /* density */
        0x0078,    /* weight */
        0x0000,    /* bounce */
        0x0040,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e56, &POKEY_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0250, &JACK_IN_THE_BOX_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x02c2, &JACK_IN_THE_BOX_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0276, &JACK_IN_THE_BOX_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0030,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x07d0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0441, &BOB_THE_FISH_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x04cb, &BOB_THE_FISH_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x046f, &BOB_THE_FISH_FORM_SIZES[0]),    /* sizes */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e6a, &BELLOW_HOT_SPOTS[0]),    /* hotspots */
        0x0000,    /* sizes */
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
    {
        0x1d80,    /* density */
        0x0064,    /* weight */
        0x0020,    /* bounce */
        0x0030,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
        part_drive_bucket    /* drive */
    },
    {
        0x3986,    /* density */
        0x03e8,    /* weight */
        0x00c0,    /* bounce */
        0x000c,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x05da, &CANNON_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0622, &CANNON_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x05f2, &CANNON_FORM_SIZES[0]),    /* sizes */
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
    {
        0x046c,    /* density */
        0x005a,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0694, &DYNAMITE_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x06b8, &DYNAMITE_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x06a0, &DYNAMITE_FORM_SIZES[0]),    /* sizes */
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
    {
        0x0000,    /* density */
        0x4e20,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e70, &BULLET_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x073c, &ELECTRIC_PLUG_FORM_STEPS[0]),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x074c, &ELECTRIC_PLUG_FORM_SIZES[0]),    /* sizes */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0799, &DYNAMITE_PLUNGER_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x07ab, &DYNAMITE_PLUNGER_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x079f, &DYNAMITE_PLUNGER_FORM_SIZES[0]),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0004,    /* point_count */
        0x0020,    /* priority */
        part_hit_dynamite_plunger,    /* hit */
        part_step_dynamite_plunger,    /* step */
        part_setup_dynamite_plunger,    /* setup */
        part_flip_dynamite_plunger,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_dynamite_plunger    /* drive */
    },
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x07ed, &FAN_FORM_STEPS[0]),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x07f5, &FAN_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e76, &FLASHLIGHT_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x08f5, &GENERATOR_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0955, &GENERATOR_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0915, &GENERATOR_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x09de, &GUN_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0a08, &GUN_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x09ec, &GUN_FORM_SIZES[0]),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x0012,    /* priority */
        part_hook_yes,    /* hit */
        part_step_gun,    /* step */
        part_setup_gun,    /* setup */
        part_flip_gun,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_gun    /* drive */
    },
    {
        0x07d0,    /* density */
        0x0009,    /* weight */
        0x0040,    /* bounce */
        0x0018,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0514,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0a52, &LIGHT_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0a6a, &LIGHT_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0a5a, &LIGHT_FORM_SIZES[0]),    /* sizes */
        { 0x02, 0xff },    /* refile_level */
        0x0000,    /* point_count */
        0x001b,    /* priority */
        part_hit_light,    /* hit */
        part_step_light,    /* step */
        part_setup_light,    /* setup */
        part_flip_light,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_light    /* drive */
    },
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0b35, &MONKEY_FORM_STEPS[0]),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x0b4f, &MONKEY_FORM_SIZES[0]),    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0009,    /* point_count */
        0x0027,    /* priority */
        part_hit_monkey,    /* hit */
        part_step_monkey,    /* step */
        part_setup_monkey,    /* setup */
        part_flip_monkey,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_monkey    /* drive */
    },
    {
        0x0960,    /* density */
        0x0064,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0008,    /* point_count */
        0x0032,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_pumpkin,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {
        0x000b,    /* density */
        0x0004,    /* weight */
        0x0080,    /* bounce */
        0x0008,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x03, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x0033,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_heart_balloon,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        part_drive_heart_balloon    /* drive */
    },
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        0x0000,    /* hotspots */
        0x0000,    /* sizes */
        { 0x04, 0xff },    /* refile_level */
        0x0007,    /* point_count */
        0x0034,    /* priority */
        part_hook_yes,    /* hit */
        part_hook_none_2a1,    /* step */
        part_setup_christmas_tree,    /* setup */
        part_hook_none_2ab,    /* flip */
        part_hook_none_2b0,    /* settle */
        ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)    /* drive */
    },
    {
        0x0ec0,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e7a, &BOXING_GLOVE_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x4650,    /* density */
        0x0708,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0c19, &ROCKET_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0c55, &ROCKET_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0c2d, &ROCKET_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0c96, &SCISSORS_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0ca2, &SCISSORS_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0c9a, &SCISSORS_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0ce2, &SOLAR_PANEL_FORM_STEPS[0]),    /* bitmaps2 */
        0x0000,    /* hotspots */
        NEAR_AT(0x0cea, &SOLAR_PANEL_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0d90, &TRAMPOLINE_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0dae, &TRAMPOLINE_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0d9a, &TRAMPOLINE_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0080,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e8e, &WINDMILL_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x0000,    /* density */
        0x0001,    /* weight */
        0x0100,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0e96, &BLAST_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x07d0,    /* density */
        0x0001,    /* weight */
        0x0000,    /* bounce */
        0x0100,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        0x0000,    /* bitmaps2 */
        NEAR_AT(0x0ea2, &MORT_THE_MOUSE_HOT_SPOTS[0]),    /* hotspots */
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
    {
        0x53b4,    /* density */
        0x6d60,    /* weight */
        0x0020,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x052a,    /* density */
        0x0005,    /* weight */
        0x00c0,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x07d0,    /* density */
        0x000c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
        0x0000,    /* bitmaps */
        NEAR_AT(0x0e12, &CANDLE_FORM_STEPS[0]),    /* bitmaps2 */
        NEAR_AT(0x0e36, &CANDLE_HOT_SPOTS[0]),    /* hotspots */
        NEAR_AT(0x0e1e, &CANDLE_FORM_SIZES[0]),    /* sizes */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x000c,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x0640,    /* density */
        0x03e8,    /* weight */
        0x0000,    /* bounce */
        0x0000,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x1d80,    /* density */
        0x03e8,    /* weight */
        0x0100,    /* bounce */
        0x0010,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0000,    /* bounce */
        0x0002,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0000,    /* bounce */
        0x0002,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0000,    /* bounce */
        0x0002,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0000,    /* bounce */
        0x0002,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x00f0,    /* max_w */
        0x00f0,    /* max_h */
        0x0020,    /* min_w */
        0x0020,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0040,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
    {
        0x0064,    /* density */
        0x008c,    /* weight */
        0x0080,    /* bounce */
        0x0020,    /* grip */
        0x0000,    /* gravity */
        0x0000,    /* max_speed */
        0x0000,    /* max_w */
        0x0000,    /* max_h */
        0x00f0,    /* min_w */
        0x00f0,    /* min_h */
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
        part_drive_kind_57    /* drive */
    }
};

struct game_path_sep GAME_PATH_SEP = { MESSAGES.path_sep };   /* DGROUP 0x1bca */
struct messages MESSAGES = {
    "\012\012NOT ENOUGH FREE MEMORY\012",    /* not_enough_free_memory */
    "\012You need at least 550k of free memory to run 'The Incredible Machine'.\012\012",    /* you_need_at_least */
    "Unable to initialize vm.",    /* unable_to_initialize_vm */
    "\012\012Thanks for playing 'The Incredible Machine'.\012The last password given to you was:  ",    /* thanks_for_playing */
    "Please select, in order, the three parts listed on page ",    /* please_select_in_order */
    " of the user's manual.",    /* of_the_users_manual */
    "VERSION NUMBER",    /* version_number */
    "This is version 1.00 of 'The Incredible Machine.'",    /* this_is_version */
    "MEMORY LOW",    /* memory_low */
    "Memory is getting low.  You can only place a few more parts.",    /* memory_is_getting_low */
    "OUT OF MEMORY",    /* out_of_memory */
    "You can't place any more parts.",    /* you_cant_place_any */
    "QUIT GAME",    /* quit_game */
    "Are you sure you want to quit the game?",    /* quit_body */
    "RESTART LEVEL",    /* restart_level */
    "Are you sure you want to clear all parts and restart this level?",    /* restart_body */
    "FREEFORM MODE",    /* freeform_mode */
    "Are you sure you want to enter freeform mode?",    /* freeform_body */
    "LEAVE FREEFORM MODE",    /* leave_freeform_mode */
    "Are you sure you want to leave freeform mode?",    /* leave_freeform_body */
    "CAN'T CHANGE GRAVITY",    /* cant_change_gravity */
    "You are only allowed to change the gravitational force in freeform mode.",    /* gravity_body */
    "CAN'T CHANGE AIR PRESSURE",    /* cant_change_air_pressure */
    "You are only allowed to change the air pressure in freeform mode.",    /* air_pressure_body */
    "OVERWRITE FILE",    /* overwrite_file */
    "File already exists.  Do you want to overwrite it?",    /* overwrite_body */
    "FILE ERROR",    /* file_error */
    "Unable to open that file for saving.",    /* cant_open_for_saving */
    "Unable to open that file for loading.",    /* cant_open_for_loading */
    "Disk is write protected or there is not enough memory on that disk to save this machine.",    /* disk_write_protected */
    "PATH ERROR",    /* path_error */
    "Unable to choose that path.",    /* path_error_body */
    "WRONG FORMAT",    /* wrong_format */
    "That file has not been saved in 'The Incredible Machine' format.",    /* wrong_format_body */
    "NEED PASSWORD",    /* need_password */
    "You need to enter the correct password in order to try this puzzle.",    /* need_password_body */
    "BAD PASSWORD",    /* bad_password */
    "That is not a valid password.",    /* bad_password_body */
    "SCORE CODE INVALID",    /* score_code_invalid */
    "That score code is invalid.  Your score will be set to zero.",    /* score_code_body */
    "<PARENT DIR>",    /* parent_dir */
    "LOAD MACHINE",    /* load_machine */
    "SAVE MACHINE",    /* save_machine */
    "LOAD",    /* load */
    "SAVE",    /* save */
    "CANCEL",    /* cancel */
    "File Name:",    /* file_name */
    "FREEFORM MODE",    /* freeform_mode_title */
    "PUZZLE ",    /* puzzle_prefix */
    " COMPLETED!",    /* completed */
    "Total bonus points: ",    /* total_bonus_points */
    "New Password",    /* new_password */
    { 0 },    /* empty */
    "(click button to continue)",    /* click_button_to_continue */
    "REPLAY SOLUTION",    /* replay_solution */
    "Do you want to advance to the next puzzle or replay your solution to this puzzle?",    /* replay_body */
    "SELECT PUZZLE",    /* select_puzzle */
    "PASSWORD",    /* password */
    "SOLVED ALL PUZZLES",    /* solved_all_puzzles */
    "You can create any type of machine that you wish to in freeform mode.",    /* freeform_hint */
    "Wow!!  INCREDIBLE Job!!!  You have solved all of the puzzles!!  Advance will take you to freeform mode.",    /* solved_all_body */
    "\\"    /* path_sep */
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
struct game_directories GAME_DIRECTORIES;   /* DGROUP 0x530b */
struct picked_machine PICKED_MACHINE;   /* DGROUP 0x52fe */
uint16_t _stklen;   /* DGROUP 0x52fc */
uint16_t stop_requested;   /* DGROUP 0x52fa  game_teardown(0) raises it; the loops above read it */
FILE     *tim_sx;   /* DGROUP 0x52f8  tim.sx's file record, which open_sound_file reads the sounds from */
struct bitmap **cursor_art;   /* DGROUP 0x52f6  mouse.bmp's list */
struct bitmap **panel_art;   /* DGROUP 0x52f4  the art set the panel's pieces come out of */
uint16_t cursor_follows;   /* DGROUP 0x52f2  restore_cursor_following is guarded by this */
uint8_t last_key;   /* DGROUP 0x52f1  the last key the screen loops took - a **byte**:
                       `cursor_follows` is at 0x52f2 */
/*
 * DGROUP 0x52ed  tim.pal: the far pointer `load_palette` answers, stored whole and
 * read whole by `set_palette_pointer` and `free_far_block`.
 */
uint8_t far *pal_tim;
/* DGROUP 0x52e9..0x52ed: nothing in the image names these four bytes. Ours. */
uint8_t DG52E9[4];
uint8_t far *pal_sierra;   /* DGROUP 0x52e5  sierra.pal */
uint8_t far *pal_black;   /* DGROUP 0x52e1  black.pal, as pal_tim */
/* **The font handle for "memofnt8.fnt"**, what `load_font` answered at
   start-up; `set_font` takes it and `game_teardown` gives its slot back. */
int16_t memo_font;   /* DGROUP 0x52df */
/* **The saved clip rectangle**, stored in descending order - 0x52dd is the
   left edge and 0x52d7 the bottom, which looks like a transcription error
   and is not. */
int16_t saved_clip_left;   /* DGROUP 0x52dd */
int16_t saved_clip_right;   /* DGROUP 0x52db */
int16_t saved_clip_top;   /* DGROUP 0x52d9 */
int16_t saved_clip_bottom;   /* DGROUP 0x52d7 */
int16_t music_now;   /* DGROUP 0x52d5  the tune opened and started, remembered */
/* **Four request-and-acknowledge words**, one per machine sound: something
   sets one to 2, and `run_machine_loop`, which does all four every frame,
   turns it to 1 and then stops the sound. */
int16_t sound_request_01;   /* DGROUP 0x52d3 */
int16_t sound_request_02;   /* DGROUP 0x52d1 */
int16_t sound_request_09;   /* DGROUP 0x52cf */
int16_t sound_request_0c;   /* DGROUP 0x52cd */
int16_t fill_colour;   /* DGROUP 0x52cb  the colour the panel and the title box are filled in */
/* **The colour the parts bin's column is cleared to**, 0x0b, filed once by
   `game_setup` and read only by `draw_machine_layer_a`, which puts it in
   both of the driver's fill colours before its two `fill_rect`s. */
int16_t bin_colour;   /* DGROUP 0x52c9 */
int16_t drop_cursor;   /* DGROUP 0x52c7  0xa on every frame the hand is not already carrying */
int16_t band_colour;   /* DGROUP 0x52c5  0xa where it would attach, -1 for no line */
int16_t anchor_y;   /* DGROUP 0x52c3 */
int16_t anchor_x;   /* DGROUP 0x52c1  the far part's anchor: its +0x1e and +0x20 plus +0x56, +0x57 */
int16_t band_y;   /* DGROUP 0x52bf */
int16_t band_x;   /* DGROUP 0x52bd  the pointer in play-area coordinates */
struct machine_parts MACHINE_PARTS;   /* DGROUP 0x521b */
struct moving_parts MOVING_PARTS;   /* DGROUP 0x5179 */
struct held_parts HELD_PARTS;   /* DGROUP 0x50d3 */
/* DGROUP 0x50cb..0x50d3: nothing in the image names these eight bytes. Ours. */
uint8_t DG50CB[8];
struct draw_layers DRAW_LAYERS;   /* DGROUP 0x50bf */
struct level_settings LEVEL_SETTINGS;   /* DGROUP 0x50af */
/* **The level's title and hint**, read from the level file by
   `load_level` when it is a level and written back by `write_level`;
   the briefing draws the title over the panel and wraps the hint into
   the box. Eighty bytes for the title is the distance to the hint; the
   hint's extent is the gap to the next record at 0x50af, and the reader
   (`game_fread_string`, a length byte then the bytes) can put at most
   255 in it. */
char level_hint[0x190];   /* DGROUP 0x4f1f, up to LEVEL_SETTINGS */
char level_title[0x50];   /* DGROUP 0x4ecf */
struct bitmap **score2_bmp;   /* DGROUP 0x4ecd  score2.bmp's - draw_odometer_digit's strips */
struct bitmap **bmp_4ecb;   /* DGROUP 0x4ecb  gp_bord.bmp's */
struct bitmap **menu_bmp;   /* DGROUP 0x4ec9  gp_menu.bmp's */
struct bitmap **icons_bmp;   /* DGROUP 0x4ec7  icons.bmp's list */
int16_t cursor;   /* DGROUP 0x4ec5 */
/* **The cursor showing, and the one the hourglass replaced.**
   `select_cursor` returns at once when the number it is given is already
   in `cursor`, `wait_cursor` files the outgoing one in `saved_cursor`
   unless it is the hourglass itself, and `restore_cursor` selects what is
   there. */
int16_t saved_cursor;   /* DGROUP 0x4ec3 */
int16_t master_level;   /* DGROUP 0x4ec1  the volume knob's setting; in tim.cfg */
int16_t playing;   /* DGROUP 0x4ebf  game_play runs while this is non-zero */
int16_t round_number;   /* DGROUP 0x4ebd  the puzzle being played; round_setup loads it */
/* **Written once and never read**, and that is the whole of what is known:
   `round_setup` stores 0 here, and the two bytes of this offset occur
   exactly once in the image - that store. A dead store of the original's,
   kept because DGROUP is compared with the original's memory. */
uint16_t word_4ebb;   /* DGROUP 0x4ebb */
int16_t level_count;   /* DGROUP 0x4eb9  how many L<n>.LEV there are */
int16_t furthest_level;   /* DGROUP 0x4eb7  how far the player has reached; in tim.cfg */
int16_t password_puzzle;   /* DGROUP 0x4eb5  the puzzle game_teardown prints a password for */
/* **How far each bonus counter has rolled**, 0 to 0x15 - one digit cell -
   and back to 0 with one off the counter's value. `start_counters` puts
   the first at -4, which is four steps of nothing before it moves, and
   `step_counters` draws the band only while the scroll is positive. The
   second reel's is never armed in the shipped game; see `step_counters`
   and STATUS.md. */
int16_t bonus_1_scroll;   /* DGROUP 0x4eb3 */
int16_t bonus_2_scroll;   /* DGROUP 0x4eb1 */
int32_t odometer_total;   /* DGROUP 0x4ead  the odometer's running total */
/* **Two 32-bit scores.** `finish_level` copies `odometer_total` into
   `banked_score` a word at a time, and each is read as one `int32_t`, by
   `score_to_code` and by the odometer. */
int32_t banked_score;   /* DGROUP 0x4ea9  what finish_level banks for the password */
/* `run_machine_loop` accumulates the ticks a frame took in `elapsed_ticks`
   and counts its frames in `machine_frames`. */
uint16_t machine_frames;   /* DGROUP 0x4ea7 */
uint16_t elapsed_ticks;   /* DGROUP 0x4ea5 */
/* **Three origin pairs**, y then x, all set to -8 by `round_setup`; which
   is which role is not established, only that the live one is the third,
   `origin_y`/`origin_x`: the play area's scroll origin, which
   `draw_part_clip` takes from world coordinates to get the screen's. */
int16_t origin_x;   /* DGROUP 0x4ea3 */
int16_t origin_y;   /* DGROUP 0x4ea1 */
int16_t origin_b_x;   /* DGROUP 0x4e9f */
int16_t origin_b_y;   /* DGROUP 0x4e9d */
int16_t origin_c_x;   /* DGROUP 0x4e9b */
int16_t origin_c_y;   /* DGROUP 0x4e99 */
uint16_t drag_offset_x;   /* DGROUP 0x4e97 */
/* **Where in the part the player took hold of it**: the pointer less the
   part's own origin, filed when a part is picked up and subtracted again
   every frame, so a part grabbed by its corner stays held by its corner.
   The y is first, which is the order the original writes them in. */
uint16_t drag_offset_y;   /* DGROUP 0x4e95 */
/* **Five deferred redraws**, one layer each: a change asks for N frames and
   gets one a frame. Counts, not flags - `game_screen_loop` decrements each
   by one rather than clearing it. */
uint16_t redraw_e;   /* DGROUP 0x4e93 */
uint16_t redraw_d;   /* DGROUP 0x4e91 */
uint16_t redraw_c;   /* DGROUP 0x4e8f */
uint16_t redraw_b;   /* DGROUP 0x4e8d */
uint16_t redraw_a;   /* DGROUP 0x4e8b */
/* **A countdown for the carried part's icon**, the same shape as a part's
   own `redraw_count`: the editor loop draws the icon and steps it down
   while it is not zero. */
uint16_t redraw_carried;   /* DGROUP 0x4e89 */
/* **Frames the loop that is running has run.** `step_loop_frames` adds one
   a frame from the intro's loop and from `run_machine_loop`, `round_setup`
   clears it, and `draw_machine_layer_f` clears it on its way in - which is
   what freezes the bin's header animation at frame 0, the one thing that
   reads it as a phase. The other reader is `goal_test_puzzle_70`, which
   wants 0x134 of them before it will pass, so on that puzzle it is
   elapsed time. Not `machine_frames`: that one `clear_machine` resets at
   every start and this one only a new round does. */
int16_t loop_frames;   /* DGROUP 0x4e87  wraps 0x2a00 to 0x1c00 */
uint16_t file_op_active;   /* DGROUP 0x4e85  GUESS: 1 around the chdir a file dialog does */
/* **The "memory is getting low" box has been shown.** Set with the box and
   cleared again only when the largest free block climbs back over 0x1770,
   which is the hysteresis that stops a machine hovering near the edge
   being told twice. */
uint16_t memory_warned;   /* DGROUP 0x4e83 */
int16_t holiday_valentine;   /* DGROUP 0x4e81  14 February - kind 33, the heart */
int16_t holiday_stpatrick;   /* DGROUP 0x4e7f  17 March    - read by nothing */
int16_t holiday_halloween;   /* DGROUP 0x4e7d  31 October  - kind 32, the pumpkin */
int16_t holiday_christmas;   /* DGROUP 0x4e7b  25 December - kind 34, the tree */
struct region *regions_play;   /* DGROUP 0x4e79  the play screen's */
struct region *regions_panel;   /* DGROUP 0x4e77  the briefing's controls */
struct region *regions_c;   /* DGROUP 0x4e75 */
struct region *regions_b;   /* DGROUP 0x4e73 */
struct region *regions_a;   /* DGROUP 0x4e71  the five region lists, heads of */
struct region *region_kept_b;   /* DGROUP 0x4e6f */
struct region *region_kept_a;   /* DGROUP 0x4e6d  two records kept on their own as well */
uint16_t round_state;   /* DGROUP 0x4e6b  the round and screen state machine's word */
/* **Which handle the pointer is on**, and the only word that says what a
   click in the play area will do. 0 is nothing, 1 to 8 are the handles
   `part_handle_at_pointer` answers - the two flips, the four resize
   corners and the two ends - and 9 is "carrying a part". Bit 0x8000 says
   the handle is engaged, so `tool & 0x7fff` is the handle and the bit is
   the drag. `cursor_for_tool` turns the nine into cursor numbers, which is
   where the name comes from. */
uint16_t tool;   /* DGROUP 0x4e69 */
uint16_t freeform;   /* DGROUP 0x4e67  1 in freeform mode - the bin is unlimited and nothing is scored - 0 on a loaded level */
struct free_lists FREE_LISTS;   /* DGROUP 0x4e4e */
