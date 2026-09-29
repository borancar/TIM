/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Making parts**: the bin's list of parts a level may use, one part made
 * from its kind's template and its kind's init routine, and the lists freed
 * again.
 *
 * The eighth and last module of the original's **code segment 0dff**, image
 * 0x1405b..0x14de0. Its data is the part templates, DGROUP 0x2966..0x2d06.
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x2966..0x2d06
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The part templates**, DGROUP 0x2966..0x2d06, the module's whole data: one
 * per kind, which `make_part` copies from, each ending in a far pointer to
 * that kind's init routine below - so the table is this module's.
 */
struct part_template g_part_templates[PART_KIND_COUNT] = {
    /* flags_06, flags_0a, set_size, size, init */
    { 0x0800, 0x0008, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_bowling_ball }, /* 0 */
    { 0x4800, 0x0000, { 0x0020, 0x0010 }, { 0x0020, 0x0010 }, part_init_platform }, /* 1 */
    { 0x4800, 0x0000, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_ramp }, /* 2 */
    { 0x4800, 0x0000, { 0x0050, 0x0020 }, { 0x0050, 0x0020 }, part_init_seesaw }, /* 3 */
    { 0x0800, 0x0008, { 0x0020, 0x0030 }, { 0x0020, 0x0030 }, part_init_balloon }, /* 4 */
    { 0x4800, 0x0000, { 0x0060, 0x0010 }, { 0x0060, 0x0010 }, part_init_conveyor }, /* 5 */
    { 0x4800, 0x0000, { 0x0030, 0x0020 }, { 0x0030, 0x0020 }, part_init_mouse_cage }, /* 6 */
    { 0x4800, 0x0008, { 0x0010, 0x0010 }, { 0x0010, 0x0010 }, part_init_pulley }, /* 7 */
    { 0x4800, 0x0000, { 0, 0 }, { 0, 0 }, part_init_belt }, /* 8 */
    { 0x0800, 0x0008, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_basketball }, /* 9 */
    { 0x4800, 0x0000, { 0, 0 }, { 0, 0 }, part_init_rope }, /* 10 */
    { 0x0800, 0x0008, { 0x0030, 0x0040 }, { 0x0030, 0x0040 }, part_init_bird_cage }, /* 11 */
    { 0x0800, 0x0008, { 0x0028, 0x0029 }, { 0x0028, 0x0027 }, part_init_pokey }, /* 12 */
    { 0x4800, 0x0000, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_jack_in_the_box }, /* 13 */
    { 0x4800, 0x0000, { 0x0020, 0x0020 }, { 0x0023, 0x0023 }, part_init_gear }, /* 14 */
    { 0x4800, 0x0000, { 0x0030, 0x0030 }, { 0x0030, 0x0030 }, part_init_bob_the_fish }, /* 15 */
    { 0x4800, 0x0008, { 0x0040, 0x0030 }, { 0x0040, 0x0030 }, part_init_bellow }, /* 16 */
    { 0x0800, 0x0008, { 0x0025, 0x0030 }, { 0x0028, 0x0030 }, part_init_bucket }, /* 17 */
    { 0x4800, 0x0008, { 0x0040, 0x0034 }, { 0x0040, 0x0034 }, part_init_cannon }, /* 18 */
    { 0x0800, 0x0008, { 0x0030, 0x001c }, { 0x0030, 0x001c }, part_init_dynamite }, /* 19 */
    { 0x0800, 0x0008, { 0x0028, 0x0007 }, { 0x0028, 0x0007 }, part_init_bullet }, /* 20 */
    { 0x4800, 0x0000, { 0x0030, 0x0020 }, { 0x0030, 0x0020 }, part_init_electric_plug }, /* 21 */
    { 0x4800, 0x0008, { 0x0087, 0x002f }, { 0x0087, 0x002f }, part_init_dynamite_plunger }, /* 22 */
    { 0x4800, 0x0008, { 0x0010, 0x0010 }, { 0x0010, 0x0010 }, part_init_hook }, /* 23 */
    { 0x4800, 0x0008, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_fan }, /* 24 */
    { 0x4800, 0x0008, { 0x0020, 0x0010 }, { 0x0020, 0x0010 }, part_init_flashlight }, /* 25 */
    { 0x4800, 0x0000, { 0x0048, 0x0020 }, { 0x0048, 0x0020 }, part_init_generator }, /* 26 */
    { 0x4800, 0x0008, { 0x0040, 0x001f }, { 0x0040, 0x001f }, part_init_gun }, /* 27 */
    { 0x0800, 0x0008, { 0x000f, 0x000f }, { 0x000f, 0x000f }, part_init_baseball }, /* 28 */
    { 0x4800, 0x0008, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_light }, /* 29 */
    { 0x4800, 0x0008, { 0x0010, 0x0025 }, { 0x0010, 0x0025 }, part_init_magnifying_glass }, /* 30 */
    { 0x4800, 0x0008, { 0x005c, 0x004f }, { 0x005c, 0x004f }, part_init_monkey }, /* 31 */
    { 0x0800, 0x0008, { 0x0027, 0x0021 }, { 0x0027, 0x0021 }, part_init_pumpkin }, /* 32 */
    { 0x0800, 0x0008, { 0x0025, 0x0027 }, { 0x0025, 0x0027 }, part_init_heart_balloon }, /* 33 */
    { 0x4800, 0x0008, { 0x0029, 0x0049 }, { 0x0029, 0x0049 }, part_init_christmas_tree }, /* 34 */
    { 0x4800, 0x0008, { 0x0030, 0x001f }, { 0x0030, 0x001f }, part_init_boxing_glove }, /* 35 */
    { 0x0800, 0x0008, { 0x0010, 0x0034 }, { 0x0010, 0x0034 }, part_init_rocket }, /* 36 */
    { 0x4800, 0x0008, { 0x0028, 0x0020 }, { 0x0028, 0x0020 }, part_init_scissors }, /* 37 */
    { 0x4800, 0x0000, { 0x0048, 0x0020 }, { 0x0048, 0x0020 }, part_init_solar_panel }, /* 38 */
    { 0x4800, 0x0000, { 0x0030, 0x001c }, { 0x0030, 0x001c }, part_init_trampoline }, /* 39 */
    { 0x4800, 0x0008, { 0x0028, 0x0030 }, { 0x0028, 0x0030 }, part_init_windmill }, /* 40 */
    { 0x0000, 0x0008, { 0, 0 }, { 0, 0 }, 0 }, /* 41 */
    { 0x0800, 0x0008, { 0x0018, 0x000b }, { 0x0018, 0x000b }, part_init_mort_the_mouse }, /* 42 */
    { 0x0800, 0x0008, { 0x0018, 0x0017 }, { 0x0018, 0x0017 }, part_init_cannon_ball }, /* 43 */
    { 0x0800, 0x0008, { 0x000f, 0x000f }, { 0x000f, 0x000f }, part_init_tennis_ball }, /* 44 */
    { 0x0800, 0x0008, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_candle }, /* 45 */
    { 0x4800, 0x0000, { 0x0020, 0x0010 }, { 0x0020, 0x0010 }, part_init_platform }, /* 46 */
    { 0x4800, 0x0000, { 0x0020, 0x0020 }, { 0x0020, 0x0020 }, part_init_corner_pipe }, /* 47 */
    { 0x4800, 0x0000, { 0x0020, 0x0010 }, { 0x0020, 0x0010 }, part_init_platform }, /* 48 */
    { 0x0800, 0x0008, { 0, 0 }, { 0, 0 }, part_init_anchor }, /* 49 */
    { 0x4800, 0x0008, { 0x0038, 0x002f }, { 0x0038, 0x002f }, part_init_motor }, /* 50 */
    { 0x0000, 0x0008, { 0, 0 }, { 0, 0 }, 0 }, /* 51 */
    { 0x0000, 0x0008, { 0, 0 }, { 0, 0 }, 0 }, /* 52 */
    { 0x0000, 0x0008, { 0, 0 }, { 0, 0 }, 0 }, /* 53 */
    { 0x0000, 0x0008, { 0, 0 }, { 0, 0 }, 0 }, /* 54 */
    { 0x0800, 0x0008, { 0x0060, 0x0010 }, { 0x0060, 0x0010 }, part_init_kind_55 }, /* 55 */
    { 0x4800, 0x0008, { 0x00d4, 0x0010 }, { 0x00d4, 0x0010 }, part_init_kind_56 }, /* 56 */
    { 0x0800, 0x0008, { 0x0020, 0x0010 }, { 0x0020, 0x0010 }, part_init_kind_57 }, /* 57 */
};

/*
 * 0x1405b
 *
 * Build the list of parts a level may use, and reset the machine's state around
 * it: the list head at DGROUP 0x50d7, the two pairs at 0x5179 and 0x521b, the
 * play area at 0x50af..0x50b5, and the two at 0x4ead.
 *
 * Parts 0 to 0x32 are all included except in three cases. **0x14, 0x29 and 0x31
 * are never included**, and are excluded by falling into a branch that leaves
 * the flag clear rather than by being tested against a list. And **0x20, 0x21
 * and 0x22 are conditional**, each on its own word - 0x4e7d, 0x4e81 and 0x4e7b -
 * which is what makes three of the parts appear only when the game says so.
 *
 * The three conditionals are written as three independent `if`s inside the same
 * branch rather than as a switch, so a part number that is not one of the three
 * reaches the end of them with its flag still clear and is left out too - which
 * cannot happen, because only those three get in there.
 *
 * The play area is 0x43,0x110 to -8,-8 - the negative pair being the origin
 * rather than a size, which is worth saying because it reads like a mistake.
 */
void build_part_list(void)
{
    struct part *rec;                   /* [bp-2] */
    int16_t si;                         /* si */
    int16_t wanted;                     /* di */

    g_placed_parts.next = g_placed_parts.prev
        = g_moving_parts.next = g_moving_parts.prev
        = g_held_parts.parts_bin.next = g_held_parts.parts_bin.prev = 0;

    for (si = 0; si < 0x33; si++) {
        wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && g_holiday_halloween != 0)
                wanted = 1;
            if (si == 0x21 && g_holiday_valentine != 0)
                wanted = 1;
            if (si == 0x22 && g_holiday_christmas != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0 && (rec = make_part(si)) != NULL)
            insert_sorted(rec, &g_held_parts.parts_bin);
    }

    g_held_parts.bin_list = (&g_held_parts.parts_bin);
    g_level_settings.bonus_1 = g_level_settings.bonus_2 = 0;
    g_level_settings.gravity = 0x43;
    g_level_settings.air = 0x110;
    g_level_settings.extent_y = g_level_settings.extent_x = -8;
    g_level_settings.tune = 0x3e9;
    g_odometer_total = 0;

    recompute_kind_physics();
}

/*
 * 0x14133
 *
 * Make one part: a 0xa2-byte record off the near heap, filled from the
 * sixteen-byte-per-part table at DGROUP 0x2966 and the bitmap list
 * `load_part_bitmap` left at 0xeba.
 *
 * The fields that come across are the part's kind at +6, its size at +0xa and
 * +0x50/+0x52, its extent at +0x44/+0x46, its bitmaps at +0x80 and a word at
 * +0x94. The two at +0x8c and +0x8e start at -1 rather than 0, which is what
 * "no link" looks like everywhere else in this game.
 *
 * Each part may also have an **init function** in the table, at +12 of its
 * entry, and a part that answers 1 from it is refused - the record is freed and
 * the answer is `NULL`, offset 0. The port dispatches that far pointer on
 * its value, as it does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
struct part *make_part(uint16_t kind)
{
    int16_t failed;                     /* [bp-2] */
    struct part *part;                  /* si */

    failed = 0;
    heap_check_or_hang();

    if ((part = (struct part *)(void *)calloc_far(1, sizeof(struct part))) == NULL) {
    fail:
        failed = 1;
        goto done;
    }

    heap_check_or_hang();

    part->kind = kind;
    part->flags_06 = g_part_templates[kind].flags_06;
    part->flags_0a = g_part_templates[kind].flags_0a;
    part->set_size.width = g_part_templates[kind].set_size.width;
    part->set_size.height = g_part_templates[kind].set_size.height;
    part->size[0].width = g_part_templates[kind].size.width;
    part->size[0].height = g_part_templates[kind].size.height;
    part->point_count = g_part_kinds[kind].point_count;
    part->start_x = 0xffff;
    part->start_y = 0xffff;
    if (g_part_templates[kind].init != NULL
        && g_part_templates[kind].init(part) == 1)
        goto fail;

    part->start_flags = part->flags_08;

    set_object_extent(part);

    part->mirror_size = part->size[0];

    heap_check_or_hang();

done:
    if (failed != 0) {
        free_part(part);
        return NULL;
    }

    return part;
}

/*
 * 0x14236 .. 0x14d42 - the **part initialisers**, fifty-one routines.
 *
 * The table of part kinds at DGROUP 0x2966 carries one far pointer each, at
 * +0x0c, and `make_part` calls it through that. Fifty-eight kind
 * slots reach fifty-one distinct routines: five kinds have no initialiser at
 * all and three - 1, 46 and 48 - share 0x14267.
 *
 * Nearly all of them are the same four steps:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `calloc_far(count, 4)` - into +0x82;
 *   3. refuse, by answering 1, if that allocation failed;
 *   4. call the part's own setup in segment 0x172c, and answer 0.
 *
 * Three skip step 2 - 0x147a7, 0x148e0 and 0x148ff call their setup with no
 * allocation. Two more skip both: 0x14aa2 and 0x14c48 only set flags and
 * bytes. And three allocate something else instead - 0x143fb and 0x1449d a
 * 0x2c-byte belt at +0x66, 0x1443d a 0x38-byte rope at +0x54 - each writing
 * the part's own address into the new record as its back-pointer.
 *
 * **They were a table until 2026-09-11**, six columns standing in for the
 * bodies: three flag words, the setup, a list of stores and a flag for
 * whether it allocated. That is not what the binary holds. The constants live
 * as immediates inside fifty-one separate functions - searching the whole
 * image for any two of them adjacent as data finds nothing - and the form
 * cost three defects, every one recorded in a comment beside it. The worst is
 * the one it could not report: the table had **forty-eight** of the fifty-one,
 * and 0x14ca0, 0x14cd9 and 0x14d0a were missing outright.
 */

/* 0x14236 */
uint16_t part_init_bowling_ball(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_big_ball(part);
    return 0;
}

/* 0x14267 */
uint16_t part_init_platform(struct part *part)
{
    part->flags_06 |= 0x0040;
    part->flags_08 |= 0x0180;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_platform(part);
    return 0;
}

/* 0x142a1 */
uint16_t part_init_ramp(struct part *part)
{
    part->flags_06 |= 0x0600;
    part->flags_08 |= 0x0080;
    part->start_form = part->form = 0x0001;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_ramp(part);
    return 0;
}

/* 0x142e6 */
uint16_t part_init_seesaw(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x000c;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_seesaw(part);
    return 0;
}

/* 0x14320 */
uint16_t part_init_balloon(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;
    part->attach[0].x = 16;
    part->attach[0].y = 47;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_balloon(part);
    return 0;
}

/* 0x14361 */
uint16_t part_init_conveyor(struct part *part)
{
    part->flags_08 |= 0x0081;
    part->start_form = part->form = 0x001c;
    part->start_direction = part->direction = 0x0000;
    part->grab.x = 59;
    part->grab_size = 0x000e;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_conveyor(part);
    return 0;
}

/* 0x143b3 */
uint16_t part_init_mouse_cage(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x0801;
    part->grab.x = 30;
    part->grab.y = 4;
    part->grab_size = 0x000c;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_mouse_cage(part);
    return 0;
}

/* 0x143fb */
uint16_t part_init_pulley(struct part *part)
{
    part->flags_08 |= 0x0004;
    part->attach[0].x = 0;
    part->attach[0].y = 8;
    part->attach[1].x = 15;
    part->attach[1].y = 8;

    if ((part->belt[0] = (calloc_far(1, sizeof(struct belt)))) == 0)
        return 1;
    part->belt[0]->owner = part;
    return 0;
}

/* 0x1443d */
uint16_t part_init_belt(struct part *part)
{
    if ((part->rope = (calloc_far(1, sizeof(struct rope)))) == 0)
        return 1;
    part->rope->owner = part;
    return 0;
}

/* 0x1446c */
uint16_t part_init_basketball(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_big_ball(part);
    return 0;
}

/* 0x1449d */
uint16_t part_init_rope(struct part *part)
{
    if ((part->belt[0] = (calloc_far(1, sizeof(struct belt)))) == 0)
        return 1;
    part->belt[0]->owner = part;
    return 0;
}

/* 0x144cb */
uint16_t part_init_bird_cage(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;
    part->attach[0].x = 21;
    part->attach[0].y = 2;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_bird_cage(part);
    return 0;
}

/* 0x1450c */
uint16_t part_init_pokey(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x8000;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_pokey(part);
    return 0;
}

/* 0x14547 */
uint16_t part_init_jack_in_the_box(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1001;
    part->grab.x = 8;
    part->grab.y = 9;
    part->grab_size = 0x000e;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_jack_in_the_box(part);
    return 0;
}

/* 0x1458f */
uint16_t part_init_gear(struct part *part)
{
    part->flags_08 |= 0x0001;
    part->grab.x = part->grab.y = 13;
    part->grab_size = 0x0008;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_big_ball(part);
    return 0;
}

/* 0x145d1 */
uint16_t part_init_bob_the_fish(struct part *part)
{
    part->flags_08 |= 0x1000;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_bob_the_fish(part);
    return 0;
}

/* 0x14607 */
uint16_t part_init_bellow(struct part *part)
{
    part->flags_06 |= 0x0400;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_bellow(part);
    return 0;
}

/* 0x1463d */
uint16_t part_init_bucket(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;
    part->attach[0].x = 18;
    part->attach[0].y = 0;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_bucket(part);
    return 0;
}

/* 0x1467e */
uint16_t part_init_cannon(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_cannon(part);
    return 0;
}

/* 0x146bd */
uint16_t part_init_dynamite(struct part *part)
{
    part->flags_06 |= 0x0420;
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_dynamite(part);
    return 0;
}

/* 0x146fc */
uint16_t part_init_bullet(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_bullet(part);
    return 0;
}

/* 0x1472d */
uint16_t part_init_electric_plug(struct part *part)
{
    part->flags_06 |= 0x0200;
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0002;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_electric_plug(part);
    return 0;
}

/* 0x1476c */
uint16_t part_init_dynamite_plunger(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_dynamite_plunger(part);
    return 0;
}

/* 0x147a7 */
uint16_t part_init_hook(struct part *part)
{
    part->flags_06 |= 0x0200;
    part->flags_08 |= 0x0004;

    part_setup_hook(part);
    return 0;
}

/* 0x147c5 */
uint16_t part_init_fan(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0001;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_fan(part);
    return 0;
}

/* 0x14804 */
uint16_t part_init_flashlight(struct part *part)
{
    part->flags_06 |= 0x0400;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_flashlight(part);
    return 0;
}

/* 0x1483a */
uint16_t part_init_generator(struct part *part)
{
    part->flags_08 |= 0x1001;
    part->flags_0a |= 0x0002;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_generator(part);
    return 0;
}

/* 0x14874 */
uint16_t part_init_gun(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_gun(part);
    return 0;
}

/* 0x148af */
uint16_t part_init_baseball(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_small_ball(part);
    return 0;
}

/* 0x148e0 */
uint16_t part_init_light(struct part *part)
{
    part->flags_06 |= 0x0200;
    part->flags_08 |= 0x1004;

    part_setup_light(part);
    return 0;
}

/* 0x148ff */
uint16_t part_init_magnifying_glass(struct part *part)
{
    part->flags_06 |= 0x0400;

    part_setup_magnifying_glass(part);
    return 0;
}

/* 0x14919 */
uint16_t part_init_monkey(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1805;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_monkey(part);
    return 0;
}

/* 0x14954 */
uint16_t part_init_pumpkin(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_pumpkin(part);
    return 0;
}

/* 0x14985 */
uint16_t part_init_heart_balloon(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;
    part->attach[0].x = 18;
    part->attach[0].y = 35;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_heart_balloon(part);
    return 0;
}

/* 0x149c6 */
uint16_t part_init_christmas_tree(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_christmas_tree(part);
    return 0;
}

/* 0x149f7 */
uint16_t part_init_boxing_glove(struct part *part)
{
    part->flags_06 |= 0x0400;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_boxing_glove(part);
    return 0;
}

/* 0x14a2d */
uint16_t part_init_rocket(struct part *part)
{
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_rocket(part);
    return 0;
}

/* 0x14a67 */
uint16_t part_init_scissors(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x1000;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_scissors(part);
    return 0;
}

/* 0x14aa2 */
uint16_t part_init_solar_panel(struct part *part)
{
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0002;

    return 0;
}

/* 0x14ab9 */
uint16_t part_init_trampoline(struct part *part)
{
    part->flags_08 |= 0x1000;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_trampoline(part);
    return 0;
}

/* 0x14aef */
uint16_t part_init_windmill(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x0801;
    part->grab.x = 15;
    part->grab.y = 15;
    part->grab_size = 0x0008;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_windmill(part);
    return 0;
}

/* 0x14b37 */
uint16_t part_init_mort_the_mouse(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x8000;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_mort_the_mouse(part);
    return 0;
}

/* 0x14b72 */
uint16_t part_init_cannon_ball(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_cannon_ball(part);
    return 0;
}

/* 0x14ba3 */
uint16_t part_init_tennis_ball(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_small_ball(part);
    return 0;
}

/* 0x14bd4 */
uint16_t part_init_candle(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x1000;
    part->flags_0a |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_candle(part);
    return 0;
}

/* 0x14c12 */
uint16_t part_init_corner_pipe(struct part *part)
{
    part->flags_06 |= 0x0600;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_corner_pipe(part);
    return 0;
}

/* 0x14c48 */
uint16_t part_init_anchor(struct part *part)
{
    part->flags_08 |= 0x0004;
    part->attach[0].x = 0;
    part->attach[0].y = 0;

    return 0;
}

/* 0x14c62 */
uint16_t part_init_motor(struct part *part)
{
    part->flags_06 |= 0x0400;
    part->flags_08 |= 0x0001;
    part->flags_0a |= 0x0001;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_motor(part);
    return 0;
}

/* 0x14ca0 */
uint16_t part_init_kind_55(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_kinds_55_57(part);
    return 0;
}

/* 0x14cd9 */
uint16_t part_init_kind_56(struct part *part)
{
    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_kind_56(part);
    return 0;
}

/* 0x14d0a */
uint16_t part_init_kind_57(struct part *part)
{
    part->flags_06 |= 0x0020;
    part->flags_08 |= 0x0004;

    if ((part->points = (calloc_far(part->point_count, 4))) == 0)
        return 1;

    part_setup_kinds_55_57(part);
    return 0;
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
    free_part_list(g_held_parts.parts_bin.next);
    free_part_list(g_placed_parts.next);
    free_part_list(g_moving_parts.next);

    g_placed_parts.next = g_moving_parts.next
        = g_held_parts.parts_bin.next = 0;
}

/*
 * 0x14d71
 *
 * Free every part on one list. The next pointer is taken out of the record
 * *before* the record is freed, which is the only way to walk a list you are
 * destroying.
 */
void free_part_list(struct part *si)
{
    /* `or si,si` at 0x14d8c: the list ends on an offset of 0, which as a
       pointer is `NULL` and never NULL. */
    while (si != NULL) {
        struct part *next = si->next;

        free_part(si);
        si = next;
    }
}

/*
 * 0x14d95
 *
 * Give a part back: its per-bitmap array, then two records it may or may not
 * own, then the part itself. Every free goes through the checked one, so a
 * corrupt heap stops here rather than later.
 *
 * The two conditions are the interesting part. The record at +0x54 is freed
 * only when bit 0 of the flags at +8 is **clear** - with it set the record
 * belongs to something else and freeing it would be a double free. And the
 * record at +0x66 is freed only for parts 7 and 0x0a, compared by number
 * rather than by a flag: two particular parts allocate it and the rest leave
 * the field as whatever it was.
 *
 * A null part is not an error; it returns.
 */
void free_part(struct part *part)
{
    if (part != NULL) {   /* the offset: `or si,si` at 0x14d9c */
        if (part->points != 0)
            checked_free(part->points);

        if (part->rope != 0
            && (part->flags_08 & 1) == 0)
            checked_free(part->rope);

        if (part->belt[0] != 0
            && (part->kind == KIND_PULLEY
                || part->kind == KIND_ROPE))
            checked_free(part->belt[0]);

        checked_free((uint8_t *)part);
    }
}
