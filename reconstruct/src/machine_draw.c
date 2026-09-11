/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Drawing the machine**: its five layers, the ropes, belts and
 * curves between parts, the part being carried, and the panels, buttons and
 * odometer around it. Part allocation lives here too, next to the drawing
 * that depends on it.
 *
 * This file corresponds to the original's **code segment 14de**, image
 * 0x14de0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include <stdio.h>

#include "tim.h"
#include "io.h"
#include "dgroup.h"

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
    int16_t si;

    DG50D3.word_50d9 = 0;
    DG50D3.bin_head_ptr = 0;
    DG5179.moving_tail_ptr = 0;
    DG5179.moving_ptr = 0;
    DG521B.parts_tail_ptr = 0;
    DG521B.parts_ptr = 0;

    for (si = 0; si < 0x33; si++) {
        int16_t wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && ((uint16_t)DG4E67.holiday_halloween) != 0)
                wanted = 1;
            if (si == 0x21 && ((uint16_t)DG4E67.holiday_valentine) != 0)
                wanted = 1;
            if (si == 0x22 && ((uint16_t)DG4E67.holiday_christmas) != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0) {
            struct part *rec = make_part((uint16_t)si);

            if (rec != 0)
                insert_sorted(rec, 0x50d7);
        }
    }

    DG50D3.bin_list_ptr = 0x50d7;
    DG50AF.bonus_b = 0;
    DG50AF.bonus_a = 0;
    DG50AF.gravity = 0x43;
    DG50AF.air = 0x110;
    DG50AF.extent_x = -8;
    DG50AF.extent_y = -8;
    DG50AF.tune = 0x3e9;
    DG4E67.counter = 0;

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
 * the answer is 0. The port dispatches that far pointer on its value, as it
 * does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
struct part *make_part(uint16_t kind)
{
    struct part *part = 0;
    uint16_t block;
    int16_t failed = 0;

    heap_check_or_hang();

    /* `heap_calloc_far` answers the offset the guest holds the block as, so
       the conversion is `PARTP` and not a cast: a cast would build a host
       pointer out of a 16-bit number. The offset is kept until the refusal
       is tested, because the guest's null is offset 0 and `PARTP(0)` is a
       real address inside DGROUP. */
    block = heap_calloc_far(1, sizeof(struct part));
    if (block == 0) {
        failed = 1;
        goto done;
    }
    part = PARTP(block);

    heap_check_or_hang();

    part->kind = kind;
    part->flags_06 = PARTTMPL(kind).flags_06;
    part->flags_0a = PARTTMPL(kind).flags_0a;
    part->word_50 = PARTTMPL(kind).word_50;
    part->word_52 = PARTTMPL(kind).word_52;
    part->width = PARTTMPL(kind).width;
    part->height = PARTTMPL(kind).height;
    part->point_count =
        PARTKIND(kind).point_count;
    part->word_8c = 0xffff;
    part->word_8e = 0xffff;
    part->word_94 = PARTTMPL(kind).init.off;

    if (!far_eq(PARTTMPL(kind).init, FAR_NULL)
        && call_part_init(PARTTMPL(kind).init, dg_off(dgroup, part)) == 1) {
        failed = 1;
        goto done;
    }

    part->word_94 = part->flags_08;

    set_object_extent(part);

    part->word_42 = ((uint16_t)part->height);
    part->word_40 = ((uint16_t)part->width);

    heap_check_or_hang();

done:
    if (failed != 0) {
        if (part != 0)
            free_part(part);
        return 0;
    }

    return part;
}

/*
 * 0x14236 .. 0x14d42 - the **part initialisers**, fifty-one routines.
 *
 * The table of part kinds at DGROUP 0x2966 carries one far pointer each, at
 * +0x0c, and `make_part` calls it through `call_part_init`. Fifty-eight kind
 * slots reach fifty-one distinct routines: five kinds have no initialiser at
 * all and three - 1, 46 and 48 - share 0x14267.
 *
 * Nearly all of them are the same four steps:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `heap_calloc_far(count, 4)` - into +0x82;
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
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0001, part);
    return 0;
}

/* 0x14267 */
uint16_t part_init_14267(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0040);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0180);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x48ab, part);
    return 0;
}

/* 0x142a1 */
uint16_t part_init_ramp(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0080);
    part->form = 0x0001;
    part->word_90 = 0x0001;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x2728, part);
    return 0;
}

/* 0x142e6 */
uint16_t part_init_seesaw(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x000c);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x40f0, part);
    return 0;
}

/* 0x14320 */
uint16_t part_init_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 16;
    part->byte_6b = 47;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x012d, part);
    return 0;
}

/* 0x14361 */
uint16_t part_init_conveyor(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0081);
    part->form = 0x001c;
    part->word_90 = 0x001c;
    part->direction = 0x0000;
    part->word_92 = 0x0000;
    part->grab_x = 59;
    part->word_58 = 0x000e;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x24d0, part);
    return 0;
}

/* 0x143b3 */
uint16_t part_init_mouse_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab_x = 30;
    part->grab_y = 4;
    part->word_58 = 0x000c;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x2ee1, part);
    return 0;
}

/* 0x143fb */
uint16_t part_init_pulley(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 0;
    part->byte_6b = 8;
    part->byte_6c = 15;
    part->byte_6d = 8;

    part->word_66 = heap_calloc_far(1, 0x2c);
    if (part->word_66 == 0)
        return 1;
    BELT(part->word_66).owner_ptr = dg_off(dgroup, part);
    return 0;
}

/* 0x1443d */
uint16_t part_init_belt(struct part *part)
{
    part->word_54 = heap_calloc_far(1, 0x38);
    if (part->word_54 == 0)
        return 1;
    ROPE(part->word_54).owner_ptr = dg_off(dgroup, part);
    return 0;
}

/* 0x1446c */
uint16_t part_init_basketball(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0001, part);
    return 0;
}

/* 0x1449d */
uint16_t part_init_rope(struct part *part)
{
    part->word_66 = heap_calloc_far(1, 0x2c);
    if (part->word_66 == 0)
        return 1;
    BELT(part->word_66).owner_ptr = dg_off(dgroup, part);
    return 0;
}

/* 0x144cb */
uint16_t part_init_bird_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 21;
    part->byte_6b = 2;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0f70, part);
    return 0;
}

/* 0x1450c */
uint16_t part_init_pokey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0c1c, part);
    return 0;
}

/* 0x14547 */
uint16_t part_init_jack_in_the_box(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->grab_x = 8;
    part->grab_y = 9;
    part->word_58 = 0x000e;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x295d, part);
    return 0;
}

/* 0x1458f */
uint16_t part_init_gear(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->grab_y = 13;
    part->grab_x = 13;
    part->word_58 = 0x0008;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0001, part);
    return 0;
}

/* 0x145d1 */
uint16_t part_init_bob_the_fish(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1be9, part);
    return 0;
}

/* 0x14607 */
uint16_t part_init_bellow(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0371, part);
    return 0;
}

/* 0x1463d */
uint16_t part_init_bucket(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 18;
    part->byte_6b = 0;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x07b2, part);
    return 0;
}

/* 0x1467e */
uint16_t part_init_cannon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0b88, part);
    return 0;
}

/* 0x146bd */
uint16_t part_init_dynamite(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0420);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1261, part);
    return 0;
}

/* 0x146fc */
uint16_t part_init_146fc(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x08a1, part);
    return 0;
}

/* 0x1472d */
uint16_t part_init_electric_plug(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1556, part);
    return 0;
}

/* 0x1476c */
uint16_t part_init_dynamite_plunger(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x3294, part);
    return 0;
}

/* 0x147a7 */
uint16_t part_init_hook(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part_setup(0x19db, part);
    return 0;
}

/* 0x147c5 */
uint16_t part_init_fan(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1a32, part);
    return 0;
}

/* 0x14804 */
uint16_t part_init_flashlight(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1d28, part);
    return 0;
}

/* 0x1483a */
uint16_t part_init_generator(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1dfb, part);
    return 0;
}

/* 0x14874 */
uint16_t part_init_gun(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x23b1, part);
    return 0;
}

/* 0x148af */
uint16_t part_init_baseball(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x00c9, part);
    return 0;
}

/* 0x148e0 */
uint16_t part_init_light(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part_setup(0x2b58, part);
    return 0;
}

/* 0x148ff */
uint16_t part_init_magnifying_glass(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part_setup(0x3030, part);
    return 0;
}

/* 0x14919 */
uint16_t part_init_monkey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1805);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x2cce, part);
    return 0;
}

/* 0x14954 */
uint16_t part_init_pumpkin(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x35f4, part);
    return 0;
}

/* 0x14985 */
uint16_t part_init_heart_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 18;
    part->byte_6b = 35;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x2682, part);
    return 0;
}

/* 0x149c6 */
uint16_t part_init_christmas_tree(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1075, part);
    return 0;
}

/* 0x149f7 */
uint16_t part_init_boxing_glove(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x065b, part);
    return 0;
}

/* 0x14a2d */
uint16_t part_init_rocket(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x3737, part);
    return 0;
}

/* 0x14a67 */
uint16_t part_init_scissors(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x389b, part);
    return 0;
}

/* 0x14aa2 */
uint16_t part_init_solar_panel(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    return 0;
}

/* 0x14ab9 */
uint16_t part_init_trampoline(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x3f72, part);
    return 0;
}

/* 0x14aef */
uint16_t part_init_windmill(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab_x = 15;
    part->grab_y = 15;
    part->word_58 = 0x0008;

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x496f, part);
    return 0;
}

/* 0x14b37 */
uint16_t part_init_mort_the_mouse(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x346f, part);
    return 0;
}

/* 0x14b72 */
uint16_t part_init_cannon_ball(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0065, part);
    return 0;
}

/* 0x14ba3 */
uint16_t part_init_tennis_ball(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x00c9, part);
    return 0;
}

/* 0x14bd4 */
uint16_t part_init_candle(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x0950, part);
    return 0;
}

/* 0x14c12 */
uint16_t part_init_corner_pipe(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x377b, part);
    return 0;
}

/* 0x14c48 */
uint16_t part_init_14c48(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->byte_6a = 0;
    part->byte_6b = 0;

    return 0;
}

/* 0x14c62 */
uint16_t part_init_motor(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1435, part);
    return 0;
}

/* 0x14ca0 */
uint16_t part_init_14ca0(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1105, part);
    return 0;
}

/* 0x14cd9 */
uint16_t part_init_14cd9(struct part *part)
{
    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x10b6, part);
    return 0;
}

/* 0x14d0a */
uint16_t part_init_14d0a(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        heap_calloc_far(part->point_count, 4);
    if (part->points_ptr == 0)
        return 1;

    part_setup(0x1105, part);
    return 0;
}

/*
 * OURS: reach one part initialiser by its image address.
 *
 * The original has no such routine. Each kind's initialiser is called through
 * the relocated far pointer at +0x0c of its entry in the table at DGROUP
 * 0x2966, and the port has no way to call one - so `call_part_init` in io.c
 * turns the pointer back into an image address and this turns that address
 * into a call. It is the same stand-in as `part_setup` and `part_finish`.
 *
 * An address with no case **aborts**: a part built by nothing at all would
 * surface much later as a level that cannot be solved.
 */
uint16_t part_init(uint32_t at, struct part *part)
{
    switch (at) {
    case 0x14236: return part_init_bowling_ball(part);
    case 0x14267: return part_init_14267(part);
    case 0x142a1: return part_init_ramp(part);
    case 0x142e6: return part_init_seesaw(part);
    case 0x14320: return part_init_balloon(part);
    case 0x14361: return part_init_conveyor(part);
    case 0x143b3: return part_init_mouse_cage(part);
    case 0x143fb: return part_init_pulley(part);
    case 0x1443d: return part_init_belt(part);
    case 0x1446c: return part_init_basketball(part);
    case 0x1449d: return part_init_rope(part);
    case 0x144cb: return part_init_bird_cage(part);
    case 0x1450c: return part_init_pokey(part);
    case 0x14547: return part_init_jack_in_the_box(part);
    case 0x1458f: return part_init_gear(part);
    case 0x145d1: return part_init_bob_the_fish(part);
    case 0x14607: return part_init_bellow(part);
    case 0x1463d: return part_init_bucket(part);
    case 0x1467e: return part_init_cannon(part);
    case 0x146bd: return part_init_dynamite(part);
    case 0x146fc: return part_init_146fc(part);
    case 0x1472d: return part_init_electric_plug(part);
    case 0x1476c: return part_init_dynamite_plunger(part);
    case 0x147a7: return part_init_hook(part);
    case 0x147c5: return part_init_fan(part);
    case 0x14804: return part_init_flashlight(part);
    case 0x1483a: return part_init_generator(part);
    case 0x14874: return part_init_gun(part);
    case 0x148af: return part_init_baseball(part);
    case 0x148e0: return part_init_light(part);
    case 0x148ff: return part_init_magnifying_glass(part);
    case 0x14919: return part_init_monkey(part);
    case 0x14954: return part_init_pumpkin(part);
    case 0x14985: return part_init_heart_balloon(part);
    case 0x149c6: return part_init_christmas_tree(part);
    case 0x149f7: return part_init_boxing_glove(part);
    case 0x14a2d: return part_init_rocket(part);
    case 0x14a67: return part_init_scissors(part);
    case 0x14aa2: return part_init_solar_panel(part);
    case 0x14ab9: return part_init_trampoline(part);
    case 0x14aef: return part_init_windmill(part);
    case 0x14b37: return part_init_mort_the_mouse(part);
    case 0x14b72: return part_init_cannon_ball(part);
    case 0x14ba3: return part_init_tennis_ball(part);
    case 0x14bd4: return part_init_candle(part);
    case 0x14c12: return part_init_corner_pipe(part);
    case 0x14c48: return part_init_14c48(part);
    case 0x14c62: return part_init_motor(part);
    case 0x14ca0: return part_init_14ca0(part);
    case 0x14cd9: return part_init_14cd9(part);
    case 0x14d0a: return part_init_14d0a(part);

    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part initialiser at %#07lx",
                 (unsigned long)at);
        not_transcribed(what);
    }
    return 1;
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
    if (part == 0)
        return;

    if (part->points_ptr != 0)
        checked_free(part->points_ptr);

    if (part->word_54 != 0
        && (part->flags_08 & 1) == 0)
        checked_free(part->word_54);

    if (part->word_66 != 0
        && (part->kind == KIND_PULLEY
            || part->kind == KIND_ROPE))
        checked_free(part->word_66);

    checked_free(dg_off(dgroup, part));
}

/*
 * 0x15004
 *
 * Draw a **scroll** of a given width with a string centred on it: the two end
 * caps and a repeating middle out of the set at DGROUP 0x52f4, and the text
 * twice, once dark and once light one pixel up and left.
 *
 * The centring is measured, not assumed - `text_width_thunk` is asked how wide
 * the string is and the difference from the scroll's width is halved - so a
 * string wider than the scroll centres to a negative offset and runs off both
 * ends rather than being clipped or wrapped.
 *
 * The middle piece is laid every 8 pixels from `x + 0x18` to `x + w - 0x18`,
 * which is what lets one scroll bitmap stretch to any width. The right cap
 * goes at that same `x + w - 0x18` - `add ax, 0xffe8` at 0x15080, the same two
 * instructions as the loop bound at 0x1506f - so it sits where the middle
 * stopped. Putting it at `x + w` left a bar of bare background between the
 * last middle piece and the cap.
 *
 * The text is drawn twice for a shadow: colour 0xf at `centre - 1, y + 6`,
 * then colour 5 at `centre, y + 5`, **the same string both times**.
 *
 * 0x150c1 is `mov ax, [bp+6]` / `inc word [bp+6]` / `push ax` - a post
 * increment, so the pointer that is pushed is the one *before* the increment
 * and the increment itself is a dead store, the routine returning three
 * instructions later. This was read as though the second pass started a
 * character later, and the briefing's title bar duly came out reading "UZZLE 1:
 * TUTORIAL" with the light pass painted over the dark one. The order of the
 * three instructions is the whole of the evidence.
 */
void draw_scroll_text(const volatile uint8_t * str, int16_t x, int16_t y, int16_t w)
{
    dg_off_t set = DG52ED.panel_art_ptr;
    int16_t  centre;
    int16_t  i;

    centre = (int16_t)(x + (w - (int16_t)text_width_thunk(str)) / 2);

    clear_flag_2d44_thunk();

    draw_bitmap(BMPP(BMPSET(set).bmp[0]), x, y, 0);

    for (i = (int16_t)(x + 0x18); i < (int16_t)(x + w - 0x18);
         i = (int16_t)(i + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x1]), i, (int16_t)(y + 2), 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0x2]),
                (int16_t)(x + w - 0x18), y, 0);

    DG3890.unknown_02 = 1;                    /* transparent: no background line */
    DG3890.unknown_00 = 0x0f;
    draw_string(str, (int16_t)(centre - 1), (int16_t)(y + 6));

    DG3890.unknown_00 = 5;
    draw_string(str, centre, (int16_t)(y + 5));

    restore_cursor_following();
}

/*
 * 0x150db
 *
 * **Draw a button**: a left cap, as many middle pieces as the word needs, a
 * right cap, and the word over them. Three bitmaps out of the set at DGROUP
 * 0x52f4 at 0x58, 0x5c and 0x60 - and `pressed` is a *word* index added to each,
 * so the pressed button is the next bitmap along from the raised one in all
 * three places rather than the same art drawn differently.
 *
 * **The width is measured and then rounded up to a multiple of 8**, which is the
 * middle piece's width; the caps sit at `x` and at `x + rounded + 8`. That
 * rounding is why `message_box` can right-align its second button by arithmetic
 * alone - the same `(w + 7) & ~7` there and here, so the two agree without
 * either asking the other.
 *
 * **The label is centred in the rounding, not in the button**: the offset is
 * half of what the rounding added, plus 8 for the left cap. So a word that
 * rounds up by seven pixels sits three to the right of where a word that rounds
 * up by one does, and both look centred because the caps absorb it.
 *
 * `pressed` moves the label as well as choosing the art - *down* two and *left*
 * one, `y + 2 * pressed` against `x - pressed`. Two different multipliers on the
 * same flag, which is what makes the word look pushed into the button rather
 * than merely moved.
 */
void draw_button(uint16_t str, uint16_t x, uint16_t y, uint16_t pressed)
{
    dg_off_t set = DG52ED.panel_art_ptr;
    int16_t  w, rounded, right, text_off, i;

    w = (int16_t)text_width_thunk(dg_ptr(dgroup, str));
    rounded = (int16_t)((w + 7) & 0xfff8);
    right = (int16_t)(x + rounded + 8);
    text_off = (int16_t)(((rounded - w) >> 1) + 8);

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    draw_bitmap(BMPP(BMPSET(set).bmp[pressed + 0x2c]),
                (int16_t)x, (int16_t)y, 0);

    for (i = (int16_t)(x + 8); i < right; i = (int16_t)(i + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[pressed + 0x2e]),
                    i, (int16_t)y, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[pressed + 0x30]),
                right, (int16_t)y, 0);

    DG3890.unknown_02 = 1;            /* transparent: no background line */
    DG3890.unknown_00 = 5;
    draw_string(dg_ptr(dgroup, str),
                (int16_t)(x + text_off - (int16_t)pressed),
                (int16_t)(y + 2 * (int16_t)pressed + 4));

    restore_cursor_following();
}

/*
 * 0x151c8
 *
 * Draw a **panel**: a tiled background inside `x,y,w,h`, a bevel around it,
 * and the ornamented border the game's menus and the copy-protection screen
 * are built out of.
 *
 * The clip box is set to the rectangle first - and `DG3890.clip_bottom` to
 * `y + h - 1`, one less, where `DG3890.clip_right` is `x + w` - so the tiling cannot
 * escape it. The background is the bitmap at +0x74 of the set the game keeps a
 * pointer to at DGROUP 0x52f4, laid down every 0x40 in both directions, which
 * is why a panel of any size costs the same tile.
 *
 * Then the clip goes back to the whole screen or to the play area, chosen by
 * whether DGROUP 0x4e6b is 0x8000 - the intro's state - and the bevel is four
 * lines: white (0xf) across the top and down the left, and 0xe then 6 for the
 * two other sides, so the panel reads as raised.
 *
 * The ornaments are the rest: a column of +0x1c every 8 pixels down the left
 * from `y + 0x13`, a row of +0x1e every 8 across the bottom from `x + 0x10`,
 * and four corner pieces at +0x14, +0x16, +0x18 and +0x1a. Every one is placed
 * by an offset from a corner rather than from the middle, which is what lets
 * the same routine draw a 0x20-wide button and a 0x220-wide panel.
 */
void draw_panel(int16_t x, int16_t y, int16_t w, int16_t h)
{
    dg_off_t set = DG52ED.panel_art_ptr;
    int16_t  i, j;

    DG3890.clip_left    = x;
    DG3890.clip_right   = (int16_t)(x + w);
    DG3890.clip_top     = y;
    DG3890.clip_bottom  = (int16_t)(y + h - 1);
    DG3890.clip_enabled = 1;

    clear_flag_2d44_thunk();

    for (j = 0; j < h; j = (int16_t)(j + 0x40))
        for (i = 0; i < w; i = (int16_t)(i + 0x40))
            draw_bitmap(BMPP(BMPSET(set).bmp[0x3a]),
                        (int16_t)(x + i), (int16_t)(y + j), 0);

    if (DG4E67.state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    DG3890.second_colour = 0x0f;
    clip_and_draw_line(x, (int16_t)(y + 1), (int16_t)(x + w), (int16_t)(y + 1));
    clip_and_draw_line((int16_t)(x + w - 1), y,
              (int16_t)(x + w - 1), (int16_t)(y + h));

    DG3890.second_colour = 0x0e;
    clip_and_draw_line(x, y, (int16_t)(x + w), y);

    DG3890.second_colour = 0x06;
    clip_and_draw_line((int16_t)(x + w), y,
              (int16_t)(x + w), (int16_t)(y + h));

    for (i = (int16_t)(y + 0x13); i < (int16_t)(y + h); i = (int16_t)(i + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0xe]), (int16_t)(x - 2), i, 0);

    for (i = (int16_t)(x + 0x10); i < (int16_t)(x + w); i = (int16_t)(i + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0xf]), i,
                    (int16_t)(y + h - 4), 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0xa]), (int16_t)(x - 7),
                (int16_t)(y - 4), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xb]), (int16_t)(x + w - 0x10),
                (int16_t)(y - 4), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xc]), (int16_t)(x - 7),
                (int16_t)(y + h - 0x10), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xd]), (int16_t)(x + w - 0x13),
                (int16_t)(y + h - 0xe), 0);

    restore_cursor_following();
}

/*
 * 0x153b8
 *
 * Draw a **sunken box**: nine pieces of art, tiled. `draw_panel` above is the
 * raised one, built out of lines and ornaments; this is the other kind, and it
 * is built out of nothing but blits.
 *
 * The pieces are all in the set at DGROUP 0x52f4: the interior at +0x56, the
 * left and right edges at +0x6c and +0x6e, the top and bottom at +0x70 and
 * +0x72, and the four corners at +0x64, +0x66, +0x68 and +0x6a.
 *
 * The tiles are 8 pixels and the corners are **16**, which is why the edges
 * inset by 8 and the corners by 16. Both loops start at 8 and stop while
 * `w - 8` is still greater, so the last tile before the far edge is skipped and
 * the edge piece covers it.
 *
 * The clip is set to the play area first, not to the box, because every piece
 * is placed rather than tiled past a boundary - so nothing here can escape and
 * nothing has to be clipped to stop it.
 */
void draw_sunken_box(int16_t x, int16_t y, int16_t w, int16_t h)
{
    dg_off_t set = DG52ED.panel_art_ptr;
    int16_t  i, j;

    set_clip_play_area();

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    for (j = 8; (int16_t)(h - 8) > j; j = (int16_t)(j + 8)) {
        for (i = 8; (int16_t)(w - 8) > i; i = (int16_t)(i + 8))
            draw_bitmap(BMPP(BMPSET(set).bmp[0x2b]),
                        (int16_t)(i + x), (int16_t)(j + y), 0);

        draw_bitmap(BMPP(BMPSET(set).bmp[0x36]), x, (int16_t)(j + y), 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x37]),
                    (int16_t)(x + w - 8), (int16_t)(j + y), 0);
    }

    for (i = 8; (int16_t)(w - 8) > i; i = (int16_t)(i + 8)) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x38]), (int16_t)(i + x), y, 0);
        draw_bitmap(BMPP(BMPSET(set).bmp[0x39]),
                    (int16_t)(i + x), (int16_t)(y + h - 8), 0);
    }

    draw_bitmap(BMPP(BMPSET(set).bmp[0x32]), x, y, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x33]), (int16_t)(x + w - 0x10), y, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x34]), x, (int16_t)(y + h - 0x10), 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x35]), (int16_t)(x + w - 0x10),
                (int16_t)(y + h - 0x10), 0);

    restore_cursor_following();
}

/*
 * 0x158c5
 *
 * **The panel that says a puzzle is finished.** A title bar, two lines of
 * text, and - unless this was the last puzzle - the password for the next one
 * with the score code appended to it.
 *
 * The two lines are built in locals rather than drawn piecewise, because
 * `draw_scroll_text` takes one string and a width: "PUZZLE " and the level
 * number and " COMPLETED!" are concatenated first, and so are "Total bonus
 * points: " and the sum of DGROUP 0x50af and 0x50b1. Both use the same
 * scratch buffer at [bp-8] for the number, one after the other.
 *
 * The password line is the level's own line of `password.txt`, and
 * `score_to_code` then appends `-XXXXX...` to it in place - so the buffer
 * holds the whole thing and is drawn once. On the last puzzle none of that
 * happens: there is no next password to give.
 *
 * "(click button to continue)" is drawn twice, black at (0xd3, 0xee) and then
 * white one pixel up and to the right, which is the drop shadow the rest of
 * this module draws the same way. The 0xd4 in the second call is a coordinate;
 * the disassembly annotates it as `black.pal` because a string happens to
 * start at that DGROUP offset.
 *
 * This routine does not wait for the click it asks for. It presents the page
 * and returns, and the caller at 0x02710 does the waiting.
 */
void show_level_complete(void)
{
    uint8_t code[40];                    /* [bp-0x6c], password and code */
    uint8_t bonus[30]; /* [bp-0x44], the second line */
    uint8_t line[30]; /* [bp-0x26], the first line */
    uint8_t num[8]; /* [bp-8],    a number as text */

    repaint_whole_screen();

    string_copy((volatile uint8_t *)line, dg_ptr(dgroup, 0x21e2 /* "PUZZLE " */));
    int_to_string(DG4E67.round_number, (volatile uint8_t *)num, 0xa);
    string_concat((volatile uint8_t *)line, (volatile uint8_t *)num);
    string_concat((volatile uint8_t *)line, dg_ptr(dgroup, 0x21ea /* " COMPLETED!" */));

    string_copy((volatile uint8_t *)bonus, dg_ptr(dgroup, 0x21f6 /* "Total bonus points: " */));
    int_to_string((int16_t)(DG50AF.bonus_a + DG50AF.bonus_b), (volatile uint8_t *)num, 0xa);
    string_concat((volatile uint8_t *)bonus, (volatile uint8_t *)num);

    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text((volatile uint8_t *)line,  0xb8, 0x80, 0xd0);
    draw_scroll_text((volatile uint8_t *)bonus, 0xb8, 0x9c, 0xd0);

    if (DG4E67.round_number < DG4E67.level_count) {
        draw_scroll_text(dg_ptr(dgroup, 0x220b /* "New Password" */), 0xb8, 0xc4, 0xd0);

        read_password_line(DG4E67.round_number, (volatile uint8_t *)code);
        score_to_code(DG4E67.counter,
                      (volatile uint8_t *)code);

        draw_scroll_text((volatile uint8_t *)code, 0xb8, 0xd8, 0xd0);
    }

    clear_flag_2d44_thunk();

    DG3890.unknown_00 = 0;
    draw_string(dg_ptr(dgroup, 0x2219 /* "(click button to continue)" */), 0xd3, 0xee);

    DG3890.unknown_00 = 0x0f;
    draw_string(dg_ptr(dgroup, 0x2219), 0xd4, 0xed);

    restore_cursor_following();
    present_back_page();
}

/*
 * 0x15a7e
 *
 * Draw one **odometer digit**: the character `c`, at `x`, scrolled by `y`.
 *
 * The ten digits are two bitmaps, not ten, and not one. `c - '0'` picks which:
 * under 5 the first, from 5 the second with 5 taken off. Each is a vertical
 * strip of five digits 0x15 pixels apart, so the digit wanted is reached by
 * drawing the whole strip at `6 - digit * 0x15 + y` and letting the clip box
 * the caller set keep the rest of it off the screen.
 *
 * That is also what makes `y` a *scroll*. A counter rolling from one value to
 * the next passes `y` from 0 to 0x15 and the strip slides a whole cell, so the
 * old digit leaves upwards as the new one arrives - which is why the two
 * bitmaps are strips in the first place.
 *
 * The pair around the drawing is the cursor: `0x0811b` takes it off the screen
 * so the blit does not capture it, `0x08125` puts it back if it was the one
 * that removed it.
 *
 * The digit is written back into the argument slot before it is used, which
 * costs a byte and reads oddly, but the original does it and a register would
 * have done.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_odometer_digit(char c, int16_t x, int16_t y)
{
    uint8_t  digit = (uint8_t)(c + 0xd0);   /* `add al, 0xd0` is `- '0'` */
    uint16_t list  = DG4E67.score2_bmp_ptr;
    int16_t  row;

    if (digit < 5) {
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        clear_flag_2d44_thunk();
        draw_bitmap(BMPP(BMPSET(list).bmp[0]), x, row, 0);
    } else {
        digit = (uint8_t)(digit + 0xfb);    /* `add al, 0xfb` is `- 5` */
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        clear_flag_2d44_thunk();
        draw_bitmap(BMPP(BMPSET(list).bmp[1]), x, row, 0);
    }

    restore_cursor_following();
}

/*
 * 0x15a2f
 *
 * Wipe the play area and draw the machine into it again - what a message box
 * needs doing behind it once it has gone.
 *
 * The driver is set up first: 0x38a8 takes the page from 0x38a2, the two bytes
 * at 0x389d and 0x389e take the colour at 0x52cb, 0x389c is set and the on/off
 * byte at 0x3893 is cleared so nothing clips. Then the area 8,8 to 0x230 by
 * 0x160 is filled - inside the frame, not the whole screen - and the machine
 * is drawn over it.
 *
 * `step_and_draw_machine(1)` rather than 0: the argument is redraw-everything,
 * so nothing is left to the dirty rectangles that have just been painted over.
 */
void redraw_machine_area(void)
{
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.fill_colour = ((uint8_t)DG52BD.fill_colour);
    DG3890.second_colour = ((uint8_t)DG52BD.fill_colour);
    DG3890.fill_enabled = 1;
    DG3890.clip_enabled = 0;

    clear_flag_2d44_thunk();
    fill_rect(8, 8, 0x230, 0x160);
    draw_machine_thunk();
    step_and_draw_machine(1);
    present_back_page();
}

/*
 * 0x15af8
 *
 * Draw the machine and everything around it, as five calls and nothing else.
 * `paint_game_screen` calls this once the play area has been cleared, so the
 * five run in the order they overlap in and none of them clears anything.
 */
void draw_machine_thunk(void)
{
    draw_machine_layer_a();
    draw_machine_layer_b();
    draw_machine_layer_c();
    draw_machine_layer_d();
    draw_machine_layer_e();
}

/*
 * 0x15dfd
 *
 * **The parts bin**: the column down the right of the screen listing the parts
 * the player has, each as its icon with a count under it.
 *
 * The list at DGROUP 0x50d3 is walked, and this is the part worth reading
 * slowly: the parts are **grouped by kind as it goes**, not counted in
 * advance. For each run, the kind is taken from +4 of the first entry, and the
 * walk continues while the next entry has the same kind, counting as it goes.
 * The entry the game has singled out - the one at 0x50d5 - is *not* counted:
 * it starts the count at 0 rather than 1 and is skipped inside the run. So the
 * number under an icon is how many are left to place, and the one being
 * carried is already gone from it.
 *
 * A run whose count comes to zero draws nothing at all, icon included.
 *
 * The count is turned into a string and centred in the 0x38-wide cell -
 * `text_width_thunk` measured, not assumed - and drawn twice for a shadow:
 * colour 0 at one pixel left and one down, then 0xe at the true place. The
 * baseline is the icon's own height plus one, and is clamped to 0x161 so a
 * tall part cannot push its number off the bottom.
 *
 * The cells are 0x34 apart and the walk stops at y = 0x134, so the bin holds
 * however many fit and the rest of the list is simply not shown.
 *
 * The two `fill_rect`s at the top clear the column in two pieces - 0x241 wide
 * by 0x37 and 0x240 by 0x103 - which overlap by a pixel in x.
 */
void draw_machine_layer_a(void)
{
    uint8_t digits[16];      /* [bp-0x10] */
    uint16_t part;
    int16_t  kind, count, y, text_x, text_y;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.clip_enabled = 1;
    set_clip_play_area();
    DG3890.fill_enabled = 1;
    DG3890.fill_colour   = ((uint8_t)DG52BD.word_52c9);
    DG3890.second_colour = ((uint8_t)DG52BD.word_52c9);

    clear_flag_2d44_thunk();
    fill_rect(0x241, 0x63, 0x37, 2);
    fill_rect(0x240, 0x65, 0x38, 0x103);
    restore_cursor_following();

    DG3890.unknown_02 = 1;                            /* transparent text */

    part = DGU16(DG50D3.bin_list_ptr);
    y    = 0x64;

    while (part != 0 && y <= 0x134) {
        uint16_t icon;

        kind = ((int16_t)PART(part).kind);
        count = (part == DG50D3.dragged_part_ptr) ? 0 : 1;

        for (;;) {
            part = PART(part).link_ptr;
            if (part == 0)
                break;
            if (((int16_t)PART(part).kind) != kind)
                break;
            if (part != DG50D3.dragged_part_ptr)
                count++;
        }

        if (count == 0)
            continue;

        clear_flag_2d44_thunk();

        icon = BMPSET(DG4E67.icons_bmp_ptr).bmp[kind];
        draw_bitmap_centred(icon, 0x240, y, 0x38, 0x2a);

        int_to_string(count, (volatile uint8_t *)digits, 10);
        text_x = (int16_t)(0x240 + (0x38 - (int16_t)text_width_thunk((volatile uint8_t *)digits)) / 2);

        text_y = (int16_t)(y + BMP(icon).height
                           + (0x2a - BMP(icon).height) / 2 + 1);
        if (text_y > 0x161)
            text_y = 0x161;

        DG3890.unknown_00 = 0;
        draw_string((volatile uint8_t *)digits, (int16_t)(text_x - 2), (int16_t)(text_y + 1));

        DG3890.unknown_00 = 0x0e;
        draw_string((volatile uint8_t *)digits, (int16_t)(text_x - 1), text_y);

        restore_cursor_following();

        y = (int16_t)(y + 0x34);
    }
}

/*
 * 0x15b16
 *
 * The play area's **top edge**: the tile at +0xc of the border set at DGROUP
 * 0x4ecb laid every 8 pixels from x = 0x10 to x = 0x22f at y = 0, then three
 * single pieces - +0 at the left, +2 at 0x230, +0x14 at 0x238.
 *
 * The run stops at 0x22f and the next piece starts at 0x230, so the tiles and
 * the corner meet exactly; the loop's `jl` is what makes the last tile land at
 * 0x228 and not overlap it.
 */
void draw_machine_layer_b(void)
{
    dg_off_t set;
    int16_t  x;

    set_clip_play_area();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x6]), x, 0, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0]), 0, 0, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x1]), 0x230, 0, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xa]), 0x238, 0, 0);

    restore_cursor_following();
}

/*
 * 0x15b9f
 *
 * The play area's **bottom edge**: the tile at +0xe laid every 8 pixels along
 * y = 0x168, then the two corners at +4 and +6 on y = 0x160 - the corners sit
 * eight pixels higher than the run they close, because they are taller.
 */
void draw_machine_layer_c(void)
{
    dg_off_t set;
    int16_t  x;

    set_clip_play_area();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x7]), x, 0x168, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0x2]), 0, 0x160, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x3]), 0x230, 0x160, 0);

    restore_cursor_following();
}

/*
 * 0x15c13
 *
 * The play area's **left edge**: the tile at +8 laid every 8 pixels *down*
 * x = 0, from y = 8 to y = 0x161, then the same two corner pieces the top and
 * bottom edges use - +0 at the top and +4 at 0x160.
 *
 * The corners are drawn three times over between the edges, once by each of
 * the three routines that meets there. Transcribed as the repetition it is.
 */
void draw_machine_layer_d(void)
{
    dg_off_t set;
    int16_t  y;

    set_clip_play_area();
    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (y = 8; y < 0x162; y = (int16_t)(y + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x4]), 0, y, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0]), 0, 0, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x2]), 0, 0x160, 0);

    restore_cursor_following();
}

/*
 * 0x15c83
 *
 * The play area's **right edge and the bin's own frame** - the last of the
 * five, and the only one that looks at the state.
 *
 * Two vertical runs: the tile at +0xa down x = 0x238 from y = 8 to 0x161, and
 * the one at +0x10 down x = 0x278 to y = 0x16e - the second runs thirteen
 * pixels further, because it closes the bin rather than the play area.
 *
 * Then the fixed pieces: +2 and +6 closing the play area's right side at
 * (0x230, 0) and (0x230, 0x160), a line in colour 0 across the top of the bin
 * from x = 0x238 to 0x27f, and +0x14 twice on x = 0x238 - at y = 0 and y =
 * 0x3b - so the same picture caps the bin at two heights.
 *
 * **The state at 0x4e6b picks one of two markers, or neither.** 0x800 draws
 * +0x50 at x = 0x248 and 0x400 draws +0x52 at x = 0x25d, both at y = 0x45, and
 * any other state draws no marker at all. That is the only thing in the five
 * layers that changes with the state.
 *
 * The last two are +0x14 again at (0x238, 0x59) - a third time - and +0x12 at
 * (0x240, 0x168).
 */
void draw_machine_layer_e(void)
{
    dg_off_t set;
    int16_t  n;

    draw_machine_layer_f();

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    set = DG4E67.bmp_4ecb_ptr;

    for (n = 8; n < 0x162; n = (int16_t)(n + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x5]), 0x238, n, 0);

    for (n = 0; n < 0x16f; n = (int16_t)(n + 8))
        draw_bitmap(BMPP(BMPSET(set).bmp[0x8]), 0x278, n, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0x1]), 0x230, 0, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x3]), 0x230, 0x160, 0);

    DG3890.second_colour = 0;
    clip_and_draw_line(0x238, 0, 0x27f, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0xa]), 0x238, 0, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xa]), 0x238, 0x3b, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0xb]), 0x23f, 0x42, 0);

    if (DG4E67.state == 0x800)
        draw_bitmap(BMPP(BMPSET(set).bmp[0x28]), 0x248, 0x45, 0);
    else if (DG4E67.state == 0x400)
        draw_bitmap(BMPP(BMPSET(set).bmp[0x29]), 0x25d, 0x45, 0);

    draw_bitmap(BMPP(BMPSET(set).bmp[0xa]), 0x238, 0x59, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x9]), 0x240, 0x168, 0);

    restore_cursor_following();
}

/*
 * 0x15faa
 *
 * The **animated header** at the top of the parts bin, clipped to
 * (0x240, 0xa)..(0x277, 0x3b) and drawn from the set at DGROUP 0x4ec9 - the
 * `gp_menu.bmp` that `game_setup` loaded and kept.
 *
 * The frame comes from the counter at 0x4e87, halved. **And the counter is set
 * to zero on the way in**, so every call draws frame 0 and the other branches
 * are dead here - they exist for a caller that does not reset it, and there
 * is none in what has been read so far. Transcribed whole rather than reduced
 * to the branch that runs, because reducing it would be writing a routine the
 * original does not have.
 *
 * Two pieces slide: the one at +2 by `((f - 4) * 2) mod 0x38` and the one at
 * +4 by `((f - 4) * 4) mod 0x38`, both from x = 0x208, and both pinned at 0
 * until the frame reaches 4. So the second moves at twice the speed of the
 * first, and neither moves at all for the first four frames.
 *
 * Then one of two figures, chosen at frame 6: below it, the picture named by
 * the tables at 0x25a2, 0x25ae and 0x25ba - index, x and y, all indexed by the
 * frame; at or above it, the frame is taken modulo 4 and the picture is +0x10
 * of the set with its position from 0x25c6 and 0x25ce. Four tables, and each
 * carries the address it came from.
 *
 * The clip is put back to the play area on the way out, which is why
 * `draw_machine_layer_e` can carry on drawing the border afterwards.
 */
void draw_machine_layer_f(void)
{
    dg_off_t set;
    int16_t  frame, slide_a, slide_b;

    DG3890.clip_enabled = 1;
    DG3890.clip_top     = 0x0a;
    DG3890.clip_bottom  = 0x3b;
    DG3890.clip_left    = 0x240;
    DG3890.clip_right   = 0x277;

    DG4E67.word_4e87 = 0;

    frame = (int16_t)(DG4E67.word_4e87 >> 1);
    slide_a = (frame >= 4) ? (int16_t)(((frame - 4) * 2) % 0x38) : 0;

    frame = (int16_t)(DG4E67.word_4e87 >> 1);
    slide_b = (frame >= 4) ? (int16_t)(((frame - 4) * 4) % 0x38) : 0;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    clear_flag_2d44_thunk();

    set = DG4E67.menu_bmp_ptr;
    draw_bitmap(BMPP(BMPSET(set).bmp[0]), 0x240, 0x0a, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x1]), (int16_t)(0x208 + slide_a), 0x1a, 0);
    draw_bitmap(BMPP(BMPSET(set).bmp[0x2]), (int16_t)(0x208 + slide_b), 0x20, 0);

    if (frame < 6) {
        /* 0x25a2 the picture, 0x25ae its x, 0x25ba its y - by frame. */
        uint16_t which = DG25A2.picture[frame];

        draw_bitmap(BMPP(BMPSET(set).bmp[which]),
                    DG25A2.picture_x[frame],
                    DG25A2.picture_y[frame], 0);
    }

    if (frame < 4) {
        draw_bitmap(BMPP(BMPSET(set).bmp[0x7]), 0x24a, 0x2a, 0);
    } else {
        int16_t f = (int16_t)(frame & 3);

        /* 0x25c6 its x and 0x25ce its y, by the frame modulo four. */
        draw_bitmap(BMPP(BMPSET(set).bmp[f + 0x8]),
                    DG25A2.sprite_x[f],
                    DG25A2.sprite_y[f], 0);
    }

    restore_cursor_following();
    set_clip_play_area();
}

/*
 * 0x15f76
 *
 * Draw a bitmap **centred in a box**: the caller gives a corner and a size,
 * and the picture's own width and height - the words at +6 and +8 of its
 * header - decide where inside it lands.
 *
 * Both halves are `sar`, an arithmetic shift, so a picture *wider* than the
 * box centres to a negative offset and hangs off both sides equally rather
 * than being pinned to the left. That is what puts a part's icon in the middle
 * of its cell in the copy-protection grid whatever size the part is.
 */
void draw_bitmap_centred(uint16_t bmp, int16_t x, int16_t y,
                         int16_t w, int16_t h)
{
    x = (int16_t)(x + (w - BMP(bmp).width) / 2);
    y = (int16_t)(y + (h - BMP(bmp).height) / 2);

    draw_bitmap(BMPP(bmp), x, y, 0);
}

/*
 * 0x160fc
 *
 * **Draw the part in your hand at the pointer**, and tell the shape allocator
 * where it went so the backdrop under it can be restored.
 *
 * The icon is the kind's entry in the list at DGROUP 0x4ec7 - the one
 * `game_intro` loaded from "icons.bmp" - indexed by the carried part's kind at
 * +4, doubled. It is drawn straight at 0x5784,0x5782, the pointer, with the
 * clip set to the play area first so it cannot spill into the panel.
 *
 * `clear_flag_2d44_thunk` **both sides of the draw**, not once: the flag is
 * cleared, the bitmap goes down, and it is cleared again. Transcribed as the
 * two calls it is.
 *
 * Then the shape: the point handed to `alloc_shape` is the pointer offset by
 * 0x4e9f and 0x4e9d - the icon's hot spot - and the extent is the bitmap's own
 * +6 and +8. Both go in as **addresses of locals**, which is why this needs a
 * guest frame: `lea ax,[bp-6]` yields a DGROUP offset the callee reads, and a
 * C local has none. See dg_alloca in dgroup.h.
 */
void draw_carried_icon(void)
{
    int16_t ext[2];                     /* [bp-0xa], [bp-8] */
    int16_t at[3];     /* [bp-6],  [bp-4]  */
    uint16_t kind, si;

    set_clip_play_area();

    kind = PART(DG50D3.dragged_part_ptr).kind;
    si = BMPSET(DG4E67.icons_bmp_ptr).bmp[kind];

    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    clear_flag_2d44_thunk();
    draw_bitmap(BMPP(si), (int16_t)((uint16_t)DG5768.pointer_x), (int16_t)((uint16_t)DG5768.pointer_y), 0);
    clear_flag_2d44_thunk();

    at[0] = (int16_t)(uint16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_b_x));
    at[1] = (int16_t)(uint16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_b_y));
    ext[0] = BMP(si).width;
    ext[1] = BMP(si).height;

    alloc_shape((volatile uint8_t *)at,
                (volatile uint8_t *)ext, 1, 2, 0);
}

/*
 * 0x16209
 *
 * **Draw the selection around a part**: the marching-ants box, the four edge
 * strips, and whichever of the six handles that part can actually use.
 *
 * `0x25d6` is a phase counter cycling 0 to 3 and back, and it is what makes
 * the border crawl: every strip is drawn offset by it, or by `4 - it`, so the
 * pattern walks by a pixel a frame. It is stepped **once per call**, at the
 * top, before anything is drawn.
 *
 * The box comes from three different places depending on kind. A rope takes
 * its link's far part and that part's +0x56/+0x57 anchor; a belt takes its
 * +0x66 record's part and the end its +0xb names, offset by -8 and -4; and
 * everything else takes the part's own +0x2a/+0x2c and +0x44/+0x46.
 *
 * **The rope arm reads `[bp-0x20]` before anything has written it.** It uses
 * it in `([bp-0x20] >> 1) < si->+0x58`, the same shape of test
 * `part_handle_at_pointer` makes against the part's +0x46 - which is what that
 * local holds on *every other* path through this routine. On the rope path it
 * is whatever the previous call left on the stack. It is transcribed as the
 * uninitialised read it is, because the alternative is inventing the height
 * the original never fetched; the C reads a local that is only assigned later,
 * which is undefined behaviour and is the point.
 *
 * Each edge is clipped to 8..0x237 and 8..0x167, and an edge that had to be
 * clipped has its strip suppressed rather than drawn short - that is what the
 * four flags are for. A box taller than 0x80 gets its vertical strips drawn
 * twice, half a screen apart, because the strip bitmap is only that tall.
 *
 * Handle 0xe is the odd one: it draws two crossed lines rather than a border,
 * in colour 0 then 0xc, which is the "no" cursor.
 *
 * The handles themselves come from `part_flip_options` through 0x50bd, the
 * same four bits `part_handle_at_pointer` tests, so what is drawn and what can
 * be grabbed cannot disagree. The corner at +0x36 is always drawn.
 *
 * Last, the box is grown by 0xc on every side - 0x18 and 0x19 on the two
 * extents, which is not symmetric and is what the original writes - and handed
 * to `alloc_shape` so the whole decoration can be lifted off again.
 */
void draw_part_selection(struct part *part, uint16_t which, uint8_t flags)
{
    int16_t at[15];    /* [bp-0x1e], [bp-0x1c] */
    int16_t ext[2];    /* [bp-0x22], [bp-0x20] */
    uint16_t si, rec, idx, bmp;
    int16_t  step, tall;
    int16_t  keep_l = 1, keep_r = 1, keep_t = 1, keep_b = 1;
    int16_t  hx, hxm, hxr, hy, hym, hyb;

    if (DG25D6.word_25d6 == 3)
        DG25D6.word_25d6 = 0;
    else
        DG25D6.word_25d6++;

    step = (int16_t)(4 - DG25D6.word_25d6);

    DG3890.page_dst_ptr = DG3890.page_back_ptr;

    if (part->kind == KIND_BELT) {
        si = ROPE(part->word_54).end_b_ptr;
        at[0] = (int16_t)(uint16_t)(((uint16_t)PART(si).box_x)
                               + PART(si).grab_x);
        at[1] = (int16_t)(uint16_t)(((uint16_t)PART(si).box_y)
                                               + PART(si).grab_y);
        ext[0] = (int16_t)PART(si).word_58;
        /* Reads ext+2 before it is written; see the comment above. */
        ext[1] = (int16_t)(((int16_t)(uint16_t)ext[1] >> 1)
             < (int16_t)PART(si).word_58)
            ? 0x0a : PART(si).word_58;
    } else if (part->kind == KIND_ROPE) {
        rec = part->word_66;
        si = BELT(rec).end_b_ptr;
        idx = ((int8_t)BELT(rec).slot_b);
        at[0] = (int16_t)(uint16_t)(((uint16_t)PART(si).box_x)
                               + PART(si).attach[idx].x - 8);
        at[1] = (int16_t)(uint16_t)(((uint16_t)PART(si).box_y)
                       + PART(si).attach[idx].y - 4);
        ext[0] = (int16_t)0x10;
        ext[1] = (int16_t)8;
    } else {
        at[1] = (int16_t)((uint16_t)part->box_y);
        at[0] = (int16_t)((uint16_t)part->box_x);
        ext[1] = (int16_t)((uint16_t)part->height);
        ext[0] = (int16_t)((uint16_t)part->width);
    }

    DG3890.clip_left = (uint16_t)((uint16_t)at[0] - ((uint16_t)DG4E67.origin_x));
    DG3890.clip_right = (uint16_t)((uint16_t)at[0] + (uint16_t)ext[0] - ((uint16_t)DG4E67.origin_x) - 1);
    DG3890.clip_top = (uint16_t)((uint16_t)at[1] - ((uint16_t)DG4E67.origin_y));
    DG3890.clip_bottom = (uint16_t)((uint16_t)at[1]
                               + (uint16_t)ext[1]
                               - ((uint16_t)DG4E67.origin_y) - 1);
    DG3890.clip_enabled = 1;

    if (DG3890.clip_left < 8)      { DG3890.clip_left = 8;     keep_l = 0; }
    if (DG3890.clip_right > 0x237)  { DG3890.clip_right = 0x237; keep_r = 0; }
    if (DG3890.clip_top < 8)      { DG3890.clip_top = 8;     keep_t = 0; }
    if (DG3890.clip_bottom > 0x167)  { DG3890.clip_bottom = 0x167; keep_b = 0; }

    if (which == 0x0e) {
        DG3890.second_colour = 0;
        clip_and_draw_line((int16_t)((uint16_t)DG3890.clip_left),
                           (int16_t)(((uint16_t)DG3890.clip_top) + 1),
                           (int16_t)((uint16_t)DG3890.clip_right),
                           (int16_t)(((uint16_t)DG3890.clip_bottom) + 1));
        clip_and_draw_line((int16_t)((uint16_t)DG3890.clip_left),
                           (int16_t)(((uint16_t)DG3890.clip_bottom) + 1),
                           (int16_t)((uint16_t)DG3890.clip_right),
                           (int16_t)(((uint16_t)DG3890.clip_top) + 1));
        DG3890.second_colour = 0x0c;
        clip_and_draw_line((int16_t)((uint16_t)DG3890.clip_left), (int16_t)((uint16_t)DG3890.clip_top),
                           (int16_t)((uint16_t)DG3890.clip_right), (int16_t)((uint16_t)DG3890.clip_bottom));
        clip_and_draw_line((int16_t)((uint16_t)DG3890.clip_left), (int16_t)((uint16_t)DG3890.clip_bottom),
                           (int16_t)((uint16_t)DG3890.clip_right), (int16_t)((uint16_t)DG3890.clip_top));
    }

    at[0] = (int16_t)(uint16_t)(((uint16_t)DG3890.clip_left) + ((uint16_t)DG4E67.origin_x));
    at[1] = (int16_t)(uint16_t)(((uint16_t)DG3890.clip_top) + ((uint16_t)DG4E67.origin_y));
    ext[0] = (int16_t)(uint16_t)(((uint16_t)DG3890.clip_right) - ((uint16_t)DG3890.clip_left) + 1);
    ext[1] = (int16_t)(uint16_t)(((uint16_t)DG3890.clip_bottom) - ((uint16_t)DG3890.clip_top) + 1);

    tall = ((int16_t)(uint16_t)ext[1] > 0x80) ? 1 : 0;

    clear_flag_2d44_thunk();

    bmp = (uint16_t)(DG52ED.cursor_art_ptr + which * 2);

    if (keep_t) {
        draw_bitmap_scaled(BMP(bmp).data.off,
                           (int16_t)((uint16_t)DG3890.clip_left),
                           (int16_t)(((uint16_t)DG3890.clip_top) - step), 8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(BMP(bmp).data.off,
                               (int16_t)((uint16_t)DG3890.clip_left),
                               (int16_t)(((uint16_t)DG3890.clip_top) - step + 0x80),
                               8, 0x88, 0);
    }

    if (keep_l)
        draw_bitmap_scaled(BMP(bmp).data.seg,
                           (int16_t)(((uint16_t)DG3890.clip_left) - DG25D6.word_25d6),
                           (int16_t)((uint16_t)DG3890.clip_top), 0x110, 1, 0);

    if (keep_r) {
        DG3890.clip_right++;
        draw_bitmap_scaled(BMP(bmp).data.off,
                           (int16_t)(((uint16_t)DG3890.clip_right) - 1),
                           (int16_t)(((uint16_t)DG3890.clip_top) - DG25D6.word_25d6),
                           8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(BMP(bmp).data.off,
                               (int16_t)(((uint16_t)DG3890.clip_right) - 1),
                               (int16_t)(((uint16_t)DG3890.clip_top) - DG25D6.word_25d6
                                         + 0x80), 8, 0x88, 0);
        DG3890.clip_right--;
    }

    if (keep_b) {
        DG3890.clip_bottom++;
        draw_bitmap_scaled(BMP(bmp).data.seg,
                           (int16_t)(((uint16_t)DG3890.clip_left) - step),
                           (int16_t)(((uint16_t)DG3890.clip_bottom) - 1), 0x110, 1, 0);
    }

    set_clip_for_mode();

    hx  = (int16_t)((uint16_t)at[0] - ((uint16_t)DG4E67.origin_x) - 12);
    hxm = (int16_t)(hx + ((int16_t)(uint16_t)ext[0] >> 1) + 6);
    hxr = (int16_t)(hx + (int16_t)(uint16_t)ext[0] + 0x0c);
    hy  = (int16_t)((uint16_t)at[1] - ((uint16_t)DG4E67.origin_y) - 11);
    hym = (int16_t)(hy + ((int16_t)(uint16_t)ext[1] >> 1) + 6);
    hyb = (int16_t)(hy + (int16_t)(uint16_t)ext[1] + 0x0c);

    DG3890.fill_enabled = 1;
    DG3890.fill_colour = 0x0f;
    DG3890.second_colour = 0x0f;

    DG50AF.flip_options = part_flip_options(part);

    draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1b]), hx, hy, 0);

    if (DG50AF.flip_options & 1) {
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1c]), hx, hym, 0);
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1c]), hxr, hym, 0);
    }
    if (DG50AF.flip_options & 2) {
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1d]), hxm, hy, 0);
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1d]), hxm, hyb, 0);
    }
    if (DG50AF.flip_options & 4)
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1e]), hx, hyb, 0);
    if (DG50AF.flip_options & 8)
        draw_bitmap(BMPP(BMPSET(DG52ED.cursor_art_ptr).bmp[0x1f]), hxr, hyb, 0);

    at[0] = (int16_t)(uint16_t)((uint16_t)at[0] - 0x0c);
    at[1] = (int16_t)(uint16_t)((uint16_t)at[1] - 0x0c);
    ext[0] = (int16_t)(uint16_t)((uint16_t)ext[0] + 0x18);
    ext[1] = (int16_t)(uint16_t)((uint16_t)ext[1] + 0x19);

    alloc_shape((volatile uint8_t *)at,
                (volatile uint8_t *)ext, flags, 2, 0);

    restore_cursor_following();
}

/*
 * 0x16181
 *
 * One frame of the machine: settle the display buckets, run the physics, draw.
 *
 * A part carries a countdown at +0x14 saying it has moved and its bucket is
 * stale. Each frame every part with a non-zero one is put back in its bucket
 * by `link_record_into_buckets` and the countdown steps down, so a part that
 * moved is re-filed for as many frames as the count says. With `redraw_all`
 * set the count is ignored and cleared instead, which is how the first frame
 * of a machine files everything at once.
 *
 * The part at DGROUP 0x50d5 - the one being dragged - is done first and then
 * skipped in the walk, so it is filed before anything can be filed on top of
 * it, and only once.
 */
void step_and_draw_machine(int16_t redraw_all)
{
    uint16_t si;

    if (DG50D3.dragged_part_ptr != 0 && PART(DG50D3.dragged_part_ptr).byte_14 != 0) {
        link_record_into_buckets(PARTP(DG50D3.dragged_part_ptr));
        PART(DG50D3.dragged_part_ptr).byte_14--;
    }

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {
        if ((redraw_all != 0 || PART(si).byte_14 != 0)
            && si != DG50D3.dragged_part_ptr)
            link_record_into_buckets(PARTP(si));

        if (redraw_all != 0)
            PART(si).byte_14 = 0;
        else if (PART(si).byte_14 != 0)
            PART(si).byte_14--;
    }

    refile_overlapping_parts();
    draw_machine(0, 0);
}

/*
 * 0x166d6
 *
 * Clear six words at DGROUP 0x50bf. The loop counts *down* from 5 and tests
 * `jge`, so index 0 is cleared too - six entries, not five. What they hold is
 * not established.
 */
void clear_layer_heads(void)
{
    int16_t i = 5;

    do {
        DG50BF.layer_head[i] = 0;
        i--;
    } while (i >= 0);
}

/*
 * 0x166ef
 *
 * Link a record into up to two buckets, and mark it linked.
 *
 * Which buckets is decided by two bytes in the record's kind entry - the same
 * 0x3a-byte table `clamp_record_pair` indexes, read here at +0x1c rather than
 * +0x0a - and a byte of 0xff means "not in this bucket". The bucket heads are
 * the six-word array at DGROUP 0x50bf, which is the array
 * `clear_layer_heads` zeroes; that the two routines agree about it is what
 * identifies it as a set of list heads.
 *
 * The insertion is at the head: the record's link at +0x74 (or +0x76 for the
 * second bucket) takes the old head and the head becomes the record. For the
 * first bucket only, the bucket number is also stored at +0x7f.
 *
 * One record is special - the one whose address is at DGROUP 0x50d5 always
 * goes into bucket 0 whatever its kind says.
 */
void link_record_into_buckets(struct part *rec)
{
    int16_t kind = ((int16_t)rec->kind);
    int16_t i;

    rec->flags_0a |= 0x20;

    for (i = 0; i < 2; i++) {
        uint8_t slot = PARTKIND(kind).refile_level[i];

        if (slot == 0xFF)
            continue;
        if (dg_off(dgroup, rec) == DG50D3.dragged_part_ptr)
            slot = 0;

        rec->layer_next[i] = DG50BF.layer_head[slot];
        DG50BF.layer_head[slot] = dg_off(dgroup, rec);
        if (i == 0)
            rec->byte_7f = slot;
    }
}


/*
 * 0x1675e
 *
 * Draw the machine: the six bucket lists, deepest first.
 *
 * The buckets are filled by `link_record_into_buckets` and each is a tree
 * walked by the byte at +0x7f exactly as `refile_overlapping_parts` walks it -
 * equal to the level takes +0x74, anything else +0x76. Every part visited has
 * bit 5 of +0x0a cleared, which is the "already in a bucket" mark, so the
 * lists are emptied by being drawn; `clear_layer_heads` at the end takes
 * the heads with them.
 *
 * A rope, kind 8, and a belt, kind 0x0a, each draw themselves; kind 0x31 draws
 * nothing at all. Everything else goes through the one blitter, which is told
 * the level as well, so a part in two buckets is drawn twice at two depths.
 *
 * The page being drawn into, VMDS 0x38a8, is set from 0x38a2 first, and the
 * clip is put back to whatever the mode wants.
 */
void draw_machine(int16_t a, int16_t b)
{
    uint8_t  v02;          /* [bp-2] the level */
    uint8_t  v01;          /* [bp-1] the counter */
    uint16_t si;

    DG3890.page_dst_ptr = DG3890.page_back_ptr;
    DG3890.clip_enabled = 1;
    set_clip_for_mode();

    for (v01 = 6; v01 != 0; v01--) {
        v02 = (uint8_t)(v01 - 1);

        for (si = DG50BF.layer_head[v02]; si != 0;
             si = (PART(si).byte_7f == v02
                   ? PART(si).word_74
                   : PART(si).word_76)) {
            PART(si).flags_0a &= 0xffdf;

            if (PART(si).kind == KIND_BELT)
                draw_rope(PARTP(si), a);
            else if (PART(si).kind == KIND_ROPE)
                draw_belt(PARTP(si), a);
            else if (PART(si).kind != KIND_ANCHOR)
                draw_part(PARTP(si), (int16_t)v02, a, b);
        }
    }

    clear_layer_heads();

}

/*
 * 0x167fa
 *
 * Draw a rope: two straight lines in colour 0, from the four points its record
 * keeps at +8 through +0x16. Two lines and not one because a rope over a pulley
 * has a corner in it; a rope with nothing at either end - +4 or +6 zero - draws
 * nothing at all.
 *
 * With `a` set all eight coordinates are scaled into the preview window first,
 * exactly as `draw_belt` and `draw_part` scale theirs.
 */
void draw_rope(struct part *part, int16_t a)
{
    /*
     * The frame really is eight words - the four points the two lines are
     * drawn between - and the original addresses them from BP downwards, so
     * `p[0]` is `[bp-2]` and `p[7]` is `[bp-0x10]`. Held as the array it is,
     * the table is the same eight words in the other order.
     */
    int16_t words[8];
    int16_t *p[8];
    uint16_t si = part->word_54;
    int32_t k;

    for (k = 0; k < 8; k++)
        p[k] = &words[7 - k];                  /* [bp-2] down to [bp-0x10] */

    if (ROPE(si).end_a_ptr == 0 || ROPE(si).end_b_ptr == 0)
        goto out;

    clear_flag_2d44_thunk();

    for (k = 0; k < 8; k++)
        *p[k] = (int16_t)((k & 1)
                          ? ROPE(si).pt[0][k >> 1].y - DG4E67.origin_y
                          : ROPE(si).pt[0][k >> 1].x - DG4E67.origin_x);

    if (a != 0) {
        for (k = 0; k < 8; k++)
            *p[k] = (int16_t)((int16_t)long_shift_right(
                mul16x16((*p[k]), a), 10)
                + ((k & 1) ? 0x48 : 0x110));
    }

    DG3890.second_colour = 0;

    clip_and_draw_line((*p[0]), (*p[1]), (*p[2]), (*p[3]));
    clip_and_draw_line((*p[4]), (*p[5]), (*p[6]), (*p[7]));

    restore_cursor_following();

out:
    return;
}

/*
 * 0x1697d
 *
 * Draw a quadratic curve through three points, by forward differences in
 * 32-bit fixed point.
 *
 * `shift` is the resolution: 1 << shift steps, and every coordinate is carried
 * shifted left by 2 * shift so the divisions come out as shifts. The second
 * difference is `p0 + p2 - 2*p1`, the first is `(p1 - p0) << (shift + 1)`, and
 * each step adds the first difference plus the second times the odd number
 * 2i + 1 - which is the standard way of stepping a parabola without a divide.
 *
 * A step that lands on the same pixel as the last draws nothing, so a slow
 * curve does not draw the same line over and over.
 *
 * The loop counter and its limit are compared as a 32-bit pair, the high words
 * signed and the low words unsigned; both are small and non-negative here, so
 * the port writes the comparison the values actually mean.
 */
void draw_curve(uint8_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2)
{
    int32_t ddx = x0 + x2 - 2 * x1;
    int32_t ddy = y0 + y2 - 2 * y1;
    int32_t dx = (int32_t)long_shift_left((uint32_t)(x1 - x0),
                                          (uint8_t)(shift + 1));
    int32_t dy = (int32_t)long_shift_left((uint32_t)(y1 - y0),
                                          (uint8_t)(shift + 1));
    int32_t s2 = (int32_t)(int16_t)(shift << 1);
    int32_t X = (int32_t)long_shift_left((uint32_t)x0, (uint8_t)s2);
    int32_t Y = (int32_t)long_shift_left((uint32_t)y0, (uint8_t)s2);
    int32_t steps = (int32_t)(int16_t)(1 << shift);
    int16_t px = (int16_t)long_shift_right(X, (uint8_t)s2);
    int16_t py = (int16_t)long_shift_right(Y, (uint8_t)s2);
    int32_t i;

    DG3890.second_colour = colour;

    for (i = 0; i <= steps; i++) {
        int16_t sx = (int16_t)long_shift_right(X, (uint8_t)s2);
        int16_t sy = (int16_t)long_shift_right(Y, (uint8_t)s2);

        if (px != sx || py != sy) {
            clip_and_draw_line(px, py, sx, sy);
            px = sx;
            py = sy;
        }

        X += dx + (int32_t)long_multiply_2((uint32_t)ddx,
                                           (uint32_t)(2 * i + 1));
        Y += dy + (int32_t)long_multiply_2((uint32_t)ddy,
                                           (uint32_t)(2 * i + 1));
    }
}

/*
 * 0x16b39
 *
 * One length of belt between two points. Slack of four or less is a straight
 * line; anything more is a curve whose middle control point is the midpoint
 * pushed **down** by the slack, so a loose belt sags.
 */
void draw_belt_segment(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t slack)
{
    int16_t mx, my;

    if (slack <= 4) {
        clip_and_draw_line(x0, y0, x1, y1);
        return;
    }

    mx = (int16_t)((int16_t)(x0 + x1) >> 1);
    my = (int16_t)(((int16_t)(y0 + y1) >> 1) + slack);

    draw_curve(DG3890.second_colour, 4, x0, mx, x1, y0, my, y1);
}

/*
 * 0x16baf
 *
 * Draw a belt: every length of it, from the part it starts at to the part it
 * ends at, following the chain of pulleys through each one's +0x5a links.
 *
 * Each end of a length is either a pulley - kind 7, whose own belt record
 * carries the tangent points at +0x14 and +0x18 - or the part the belt is
 * fastened to, whose points come from the belt record itself. The two cases
 * differ in more than the source: an end that is *not* a pulley sets the flag
 * that makes the length sag, because that is the length whose slack was
 * measured. A length between two pulleys is drawn straight.
 *
 * With `a` set every point is scaled into the preview window before drawing,
 * and the little cap bitmap that marks a fastening is left off.
 */
void draw_belt(struct part *part, int16_t a)
{
    uint16_t v0e;       /* [bp-0x0e] the belt */
    int16_t  v0c;       /* [bp-0x0c] the slack */
    int16_t  v0a;       /* [bp-0x0a] sags */
    int16_t  v08;       /* [bp-8]  y1 */
    int16_t  v06;       /* [bp-6]  x1 */
    int16_t  v04;       /* [bp-4]  y0 */
    int16_t  v02;       /* [bp-2]  x0 */
    uint16_t di, si;

    v0e = part->word_66;

    di = BELT(v0e).end_a_ptr;
    si = PART(di).link[BELT(v0e).slot_a];
    if (si == 0)
        si = BELT(v0e).end_b_ptr;

    while (di != 0 && si != 0) {
        v0a = 0;

        if (PART(di).kind == KIND_PULLEY) {
            v02 = (int16_t)(
                BELT(PART(di).word_66).pt[0][1].x
                - DG4E67.origin_x);
            v04 = (int16_t)(
                BELT(PART(di).word_66).pt[0][1].y
                - DG4E67.origin_y);
        } else {
            v02 = (int16_t)(BELT(v0e).pt[0][0].x
                                  - DG4E67.origin_x);
            v04 = (int16_t)(BELT(v0e).pt[0][0].y
                                  - DG4E67.origin_y);
            v0a = 1;
        }

        if (PART(si).kind == KIND_PULLEY) {
            v06 = (int16_t)(
                BELT(PART(si).word_66).pt[0][0].x
                - DG4E67.origin_x);
            v08 = (int16_t)(
                BELT(PART(si).word_66).pt[0][0].y
                - DG4E67.origin_y);
        } else {
            v06 = (int16_t)(BELT(v0e).pt[0][1].x
                                  - DG4E67.origin_x);
            v08 = (int16_t)(BELT(v0e).pt[0][1].y
                                  - DG4E67.origin_y);
            v0a = 1;
        }

        if (a != 0) {
            v02 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v02, a), 10) + 0x110);
            v04 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v04, a), 10) + 0x48);
            v06 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v06, a), 10) + 0x110);
            v08 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v08, a), 10) + 0x48);
        }

        DG3890.second_colour = 6;
        clear_flag_2d44_thunk();

        if (v0a != 0) {
            v0c = link_slack(PARTP(di), v0e, 3);
            draw_belt_segment(v02, v04, v06, v08,
                              v0c);
        } else {
            clip_and_draw_line(v02, v04, v06, v08);
        }

        if (a == 0) {
            if (PART(di).kind != KIND_ANCHOR
                && PART(di).kind != KIND_PULLEY)
                draw_bitmap(BMPP(BMPSET(DG4E67.bmp_4ecb_ptr).bmp[0x24]),
                            (int16_t)(v02 - 5),
                            (int16_t)(v04 - 2), 0);

            if (PART(si).kind != KIND_ANCHOR
                && PART(si).kind != KIND_PULLEY)
                draw_bitmap(BMPP(BMPSET(DG4E67.bmp_4ecb_ptr).bmp[0x24]),
                            (int16_t)(v06 - 5),
                            (int16_t)(v08 - 2), 0);
        }

        restore_cursor_following();

        di = si;
        if (PART(di).kind == KIND_PULLEY)
            si = PART(si).link_right;
        else
            si = 0;
    }

}

/*
 * 0x16db1
 *
 * Draw one part, at one level, either scaled or not.
 *
 * `a` and `b` are the scale: zero means "no scaling" and everything goes
 * through `draw_bitmap` at the position it was worked out at; anything else
 * multiplies each coordinate by `a` and each size by `b`, takes ten fractional
 * bits off, and offsets into the scaled window at 0x110, 0x48 - so the same
 * routine draws the play area and the small preview.
 *
 * There are two ways a part is made of bitmaps, and bit 6 of the flags at +6
 * chooses between them.
 *
 * **Tiled.** The part's size at +0x44 and +0x46 in sixteenths gives a grid,
 * and the bitmap for each cell is picked from the eight around the form: a
 * single row runs form, form+px+1, form+3 left to right; a single column runs
 * form+4, form+py+5, form+7 top to bottom; anything else stays on the form
 * itself. `px` and `py` start from bit 4 of the position and flip every cell,
 * so a run of middle tiles alternates between two bitmaps rather than
 * repeating one.
 *
 * **Listed.** A chain of records, each holding the level it draws at, up to
 * four frame numbers, and an x,y offset for each - `krec[0x16]` indexed by the
 * form when bit 12 of +8 is set, and otherwise a single record built in DGROUP
 * at 0x124 out of the form, the level and the kind's own adjustment. A record
 * whose level does not match is skipped, unless this is the part being dragged
 * at DGROUP 0x50d5, which always draws. Bits 4 and 5 of +8 mirror the part
 * horizontally and vertically: the offset is measured from the far edge
 * instead, and the mirror is passed on to the blitter in the mode word.
 */
void draw_part(struct part *part, int16_t level, int16_t a, int16_t b)
{
    uint16_t v2a;   /* [bp-0x2a] the bitmap */
    uint16_t v28;   /* [bp-0x28] the record */
    uint16_t v26;   /* [bp-0x26] the kind's record */
    uint16_t v24;   /* [bp-0x24] the adjustment */
    const struct byte_pair *hot;
    uint8_t  v21;   /* [bp-0x21] the frame */
    uint16_t v20;   /* [bp-0x20] py */
    uint16_t v1e;   /* [bp-0x1e] px */
    uint16_t v1c;   /* [bp-0x1c] the bitmap index */
    uint16_t v1a;   /* [bp-0x1a] the mirror flags */
    int16_t  v18;   /* [bp-0x18] */
    int16_t  v16;   /* [bp-0x16] rows */
    int16_t  v14;   /* [bp-0x14] columns */
    int16_t  v12;   /* [bp-0x12] */
    int16_t  v10;   /* [bp-0x10] */
    int16_t  v0e;   /* [bp-0x0e] */
    int16_t  v0c;   /* [bp-0x0c] */
    int16_t  v0a;   /* [bp-0x0a] y */
    int16_t  v08;   /* [bp-8] x */
    int16_t  v06;   /* [bp-6] the column */
    uint16_t v04;   /* [bp-4] the form */
    uint16_t v02;   /* [bp-2] the kind */
    int16_t di;

    v02 = part->kind;
    v04 = part->form;
    v26 = (uint16_t)(0x0ea6 + 0x3a * (int16_t)((int16_t)v02));

    v24 = PARTKIND_AT(v26).word_18;
    hot = POINT_TABLE(v24);                    /* the hot spot by form, if the kind has them */

    clear_flag_2d44_thunk();

    if (part->flags_06 & 0x40) {
        v14 = (int16_t)(part->width >> 4);
        v16 = (int16_t)(part->height >> 4);

        v18 = (int16_t)(part->pos_x - DG4E67.origin_x);
        v0a = (int16_t)(part->pos_y - DG4E67.origin_y);

        if (v24 != 0) {
            v18 = (int16_t)(v18 + (int8_t)hot[v04].x);
            v0a = (int8_t)hot[v04].y;
        }

        v1e = (int16_t)((v18 & 0x10) >> 4);
        v20 = (int16_t)((v0a & 0x10) >> 4);
        v1c = v04;

        for (di = 0; di < v16; di++,
             v0a = (int16_t)(v0a + 0x10),
             v20 ^= 1) {
            for (v06 = 0, v08 = v18;
                 v06 < v14;
                 v06++,
                 v08 = (int16_t)(v08 + 0x10),
                 v1e ^= 1) {
                if (v16 == 1) {
                    if (v06 == 0)
                        v1c = v04;
                    else if ((int16_t)(v14 - 1) == v06)
                        v1c = (uint16_t)(v04 + 3);
                    else
                        v1c = (uint16_t)(v04 + v1e + 1);
                } else if (v14 == 1) {
                    if (di == 0)
                        v1c = (uint16_t)(v04 + 4);
                    else if ((int16_t)(v16 - 1) == di)
                        v1c = (uint16_t)(v04 + 7);
                    else
                        v1c = (uint16_t)(v04 + v20 + 5);
                }

                {
                    uint16_t bmp = BMPSET(PARTKIND_AT(v26).bitmaps_ptr).bmp[v1c];

                    if (a != 0) {
                        v0c = (int16_t)long_shift_right(
                            mul16x16(0x10, b), 10);
                        v0e = (int16_t)long_shift_right(
                            mul16x16(0x10, b), 10);
                        v10 = (int16_t)((int16_t)long_shift_right(
                            mul16x16(v08, a), 10) + 0x110);
                        v12 = (int16_t)((int16_t)long_shift_right(
                            mul16x16(v0a, a), 10) + 0x48);

                        draw_bitmap_scaled(bmp, v10, v12,
                                           v0c, v0e, 0);
                    } else {
                        draw_bitmap(BMPP(bmp), v08, v0a, 0);
                    }
                }
            }
        }

        goto done;
    }

    if (part->flags_08 & 0x1000) {
        v28 = OFF_TABLE(PARTKIND_AT(v26).bitmaps2_ptr)[v04];
    } else {
        v28 = 0x124;
        DG0124.frame[0] = (uint8_t)v04;
        DG0124.level = (uint8_t)level;

        if (v24 != 0) {
            DG0124.offset[0].x = hot[v04].x;
            DG0124.offset[0].y = hot[v04].y;
        } else {
            DG0124.offset[0].y = 0;
            DG0124.offset[0].x = 0;
        }
    }

    while (v28 != 0) {
        if (DRAWSTEP(v28).level != (uint8_t)level
            && dg_off(dgroup, part) != DG50D3.dragged_part_ptr)
            goto next;

        v21 = DRAWSTEP(v28).frame[0];

        for (di = 0; ; di++) {
            v2a = BMPSET(PARTKIND_AT(v26).bitmaps_ptr).bmp[v21];

            v08 = (int16_t)(part->pos_x - DG4E67.origin_x);
            v0a = (int16_t)(part->pos_y - DG4E67.origin_y);

            if (part->flags_08 & 0x10) {
                v08 = (int16_t)(
                    v08
                    + (((int16_t)part->word_40)
                       - (int8_t)DRAWSTEP(v28).offset[di].x
                       - BMP(v2a).width));
                v1a = 2;
            } else {
                v08 = (int16_t)(
                    v08
                    + (int8_t)DRAWSTEP(v28).offset[di].x);
                v1a = 0;
            }

            if (part->flags_08 & 0x20) {
                v0a = (int16_t)(
                    v0a
                    + (((int16_t)part->word_42)
                       - (int8_t)DRAWSTEP(v28).offset[di].y
                       - BMP(v2a).height));
                v1a |= 1;
            } else {
                v0a = (int16_t)(
                    v0a
                    + (int8_t)DRAWSTEP(v28).offset[di].y);
            }

            if (a != 0) {
                v0c = (int16_t)long_shift_right(
                    mul16x16(BMP(v2a).width, b), 10);
                v0e = (int16_t)long_shift_right(
                    mul16x16(BMP(v2a).height, b), 10);
                v10 = (int16_t)((int16_t)long_shift_right(
                    mul16x16(v08, a), 10) + 0x110);
                v12 = (int16_t)((int16_t)long_shift_right(
                    mul16x16(v0a, a), 10) + 0x48);

                draw_bitmap_scaled(v2a, v10, v12,
                                   v0c, v0e, v1a);
            } else {
                draw_bitmap(BMPP(v2a), v08, v0a, v1a);
            }

            v21 = DRAWSTEP(v28).frame[di + 1];

            if (di + 1 >= 4 || v21 == 0xff)
                break;
        }

    next:
        v28 = DRAWSTEP(v28).next;
    }

done:
    if (((int16_t)DG4E67.state) == 0x2000 && part->kind == KIND_MAGNIFYING_GLASS)
        draw_part_extra(part);

    restore_cursor_following();

}

/*
 * 0x171b5
 *
 * The extra a kind-0x1e part draws while the machine is in state 0x2000: a
 * three-point outline in colour 0x0e from the part it is linked to at +0x62,
 * and then the rectangle that outline covers registered as a shape so it gets
 * erased again.
 *
 * A part with nothing at +0x62 draws nothing.
 *
 * The three points share their x - the part's own left or right edge, chosen
 * by the mirror bit 4 of +8 - except for the middle one, which reaches across
 * to the linked part at its +0x72, +0x73 offset. So it is a bracket rather
 * than a triangle, which is why the bounding box is worked out from the
 * extremes rather than from all three.
 */
void draw_part_extra(struct part *part)
{
    /*
     * **The frame, as the original reserves it.** `sub sp,0x14` at 0x171b5,
     * which `tools/frames.py` checks. Laying the locals out inside one array
     * rather than as separate C variables is what keeps them adjacent, and
     * adjacency is not incidental here: the three x's and the three y's are
     * arrays this routine hands to `draw_polygon` by address.
     */
    int16_t size[2];  /* [bp-0x14], [bp-0x12] */
    int16_t corner[2];  /* [bp-0x10], [bp-0x0e] */
    int16_t y[3];  /* [bp-0x0c] .. [bp-8]  */
    int16_t x[3];  /* [bp-6] .. [bp-2]     */
    uint16_t di = part->linked_a;
    int16_t edge;

    if (di == 0)
        goto out;

    DG3890.fill_colour = 0x0e;
    DG3890.second_colour = 0x0e;

    x[1] = (int16_t)(PART(di).pos_x
                          + PART(di).byte_72 - DG4E67.origin_x);
    y[0] = (int16_t)(part->pos_y + 6 - DG4E67.origin_y);
    y[1] = (int16_t)(PART(di).pos_y
                          + PART(di).byte_73 - DG4E67.origin_y);
    y[2] = (int16_t)(part->pos_y + 0x10 - DG4E67.origin_y);

    if (part->flags_08 & 0x10)
        edge = (int16_t)(part->pos_x - 1);
    else
        edge = (int16_t)(part->pos_x + 0x0f);

    edge = (int16_t)(edge - DG4E67.origin_x);
    x[2] = edge;
    x[0] = edge;

    draw_polygon(3, x, y);

    if (x[0] < x[1]) {
        corner[0] = x[0];
        size[0] = (int16_t)(x[1] - x[0]);
    } else {
        corner[0] = x[1];
        size[0] = (int16_t)(x[0] - x[1]);
    }
    size[0]++;

    corner[1] = y[0] < y[1] ? y[0] : y[1];

    size[1] = (int16_t)((y[2] >= y[1] ? y[2] : y[1])
                          - corner[1] + 1);

    corner[0] = (int16_t)(corner[0] + DG4E67.origin_b_x);
    corner[1] = (int16_t)(corner[1] + DG4E67.origin_b_y);

    alloc_shape((const uint8_t *)corner, (const uint8_t *)size,
                1, 2, 0);

out:
}
