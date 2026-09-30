/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The bellows**: its hit, setup, flip and step.
 *
 * The third module of the original's **code segment 172c**, image
 * 0x175f2..0x17812 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3192..0x31e6
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3192..0x319e. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_3192[6] = {
    { 0x00, 0x00 }, { 0x2c, 0x12 }, { 0x3f, 0x14 }, { 0x3f, 0x1b },
    { 0x2c, 0x1d }, { 0x00, 0x2f },
};

/*
 * DGROUP 0x319e..0x31aa. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_319e[6] = {
    { 0x00, 0x0a }, { 0x2c, 0x12 }, { 0x3f, 0x14 }, { 0x3f, 0x1b },
    { 0x2c, 0x1d }, { 0x00, 0x25 },
};

/*
 * DGROUP 0x31aa..0x31b6. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_31aa[6] = {
    { 0x00, 0x0f }, { 0x2c, 0x12 }, { 0x3f, 0x14 }, { 0x3f, 0x1b },
    { 0x2c, 0x1d }, { 0x00, 0x20 },
};

/*
 * DGROUP 0x31b6..0x31bc. **Which table of points, by form**: a near pointer each.
 */
struct point8 *g_bellow_point_table_31b6[3] = {
    g_bellow_points_3192, g_bellow_points_319e, g_bellow_points_31aa,
};

/*
 * DGROUP 0x31bc..0x31c8. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_31bc[6] = {
    { 0x00, 0x14 }, { 0x13, 0x12 }, { 0x3f, 0x00 }, { 0x3f, 0x2f },
    { 0x13, 0x1d }, { 0x00, 0x1b },
};

/*
 * DGROUP 0x31c8..0x31d4. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_31c8[6] = {
    { 0x00, 0x14 }, { 0x13, 0x12 }, { 0x47, 0x0a }, { 0x47, 0x25 },
    { 0x13, 0x1d }, { 0x00, 0x1b },
};

/*
 * DGROUP 0x31d4..0x31e0. Connection points, 6 pairs.
 */
struct point8 g_bellow_points_31d4[6] = {
    { 0x00, 0x14 }, { 0x13, 0x12 }, { 0x47, 0x0f }, { 0x47, 0x20 },
    { 0x13, 0x1d }, { 0x00, 0x1b },
};

/*
 * DGROUP 0x31e0..0x31e6. **Which table of points, by form**: a near pointer each.
 */
struct point8 *g_bellow_point_table_31e0[3] = {
    g_bellow_points_31bc, g_bellow_points_31c8, g_bellow_points_31d4,
};

/*
 * 172c:0332, image 0x175f2 - kind 16's hit test.
 *
 * Something has touched the bellows, and this decides whether that touch
 * squeezes it. The collision record is the argument; +0x84 is the bellows
 * itself and +0x8a is which of its faces was struck.
 *
 * Bit 4 of the bellows' flags at +8 is which way round it is, and it accepts a
 * different pair of faces in each form - 1 or 3 mirrored, 0 or 4 upright. A
 * face that counts sets +0x12 to 1, which is `part_step_bellow` below squeezing.
 *
 * It answers 1 either way: the hit is a hit whether or not it worked the
 * bellows. The original's `jmp` to the next instruction at 0x1762b is the
 * compiler leaving a return path in that nothing needed.
 */
uint16_t part_hit_bellow(struct part *part)
{
    struct part *other = part->contact;
    int16_t  face = ((int16_t)part->contact_edge);

    if ((other->state & STATE_FLIP_HORIZONTAL) != 0) {
        if (face == 1 || face == 3)
            other->direction = 1;
    } else {
        if (face == 0 || face == 4)
            other->direction = 1;
    }

    return 1;
}

/*
 * 172c:0371, image 0x17631 - a setup.
 *
 * Six connection points copied out of a table chosen two ways: bit 4 of +8
 * picks between the pointer arrays at DGROUP 0x31e0 and 0x31b6, and the form
 * at +0x0c indexes the one picked. **Those two are arrays of near pointers,
 * not of points** - the load is a word - so the table this ends up walking is
 * wherever the pointer says.
 *
 * Two bytes a point at the source, four at the destination, as everywhere.
 */
void part_setup_bellow(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_bellow_point_table_31e0[part->form];
    else
        src = g_bellow_point_table_31b6[part->form];

    for (i = 0, dst = part->points; i < 6; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:03d2, image 0x17692 - kind 16's flip.
 *
 * Turn the part over and rebuild it: bit 4 of the flags at +8 is which way it
 * faces, and the setup at 172c:0371 reads that bit to pick which of its two
 * tables of connection points to copy. So the flip is the xor and then the
 * setup, and everything else follows from the points changing.
 */
void part_flip_bellow(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;

    part_setup_bellow(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:0405, image 0x176c5 - kind 16's step. **The bellows.**
 *
 * +0x12 is which way it is going - 1 squeezing, -1 opening - and +0x0c is how
 * far, over three frames 0, 1, 2. Squeezing stops at 2 and opening stops at 0,
 * so both ends simply do nothing rather than wrapping.
 *
 * **Only the squeeze blows.** `link_nearby_objects` is asked for what is in a
 * box in front of the nozzle - 0x80 wide the way the part faces, ten above -
 * and bit 4 of +8 is which side that is, which is why the two arms differ only
 * in the sign of the margin and of the push. The push is 0x800, or -0x800
 * mirrored.
 *
 * Each object found is moved if bit 12 of its own flags at +6 says the air
 * reaches it. What it gets is the push scaled two ways: by `0x100 - |+0x7a|`,
 * so a thing side-on to the draught takes the full shove and one edge-on takes
 * little, and then divided by its kind's weight at +0x0 of the kind record.
 * The product is taken in 32 bits and shifted right eight before the divide -
 * that shift is the 0x100 the first scale is out of - and only the low word of
 * the quotient is kept, which is the original's own truncation and not ours.
 *
 * Two kinds are told rather than pushed. Kind 45 has whatever it was doing
 * cancelled - +0x9c, +0x12 and +0x0c all cleared - and kind 40, if the air did
 * *not* reach it, is switched on for 0x14 steps, which is `part_step_windmill`
 * below counting down. So the bellows both blows things and trips things.
 *
 * A frame that differs from the one last drawn at +0x0e is rebuilt through the
 * same setup the flip uses, and the two ends of the travel - 0 and 2 - are
 * where the sound plays.
 */
void part_step_bellow(struct part *part)
{
    struct part *di;
    int16_t push;                       /* [bp-2] */
    int16_t v;                          /* [bp-4] */
    int16_t scale;                      /* [bp-6] */
    int32_t force;                      /* [bp-0xa] */

    part->state |= STATE_STEPPED;

    if (part->direction == 1) {
        if (part->form != 2) {
            part->form++;

            if (part->state & STATE_FLIP_HORIZONTAL) {
                link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), -0x80, 0, -10, 0);
                push = (int16_t)0xf800;
            } else {
                link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), 0, 0x80, -10, 0);
                push = 0x0800;
            }

            for (di = part->next_linked; di != NULL;
                 di = di->next_linked) {
                if (di->traits & TRAIT_IN_MOVING_LIST) {
                    scale = 0x100 - abs(di->link_dx);
                    force = mul16x16(push, scale);
                    force >>= 8;
                    v = force / g_part_kinds[di->kind].weight;
                    di->vel_x += v;

                    clamp_record_pair(di);

                    if (di->kind == 0x2d)
                        di->form = di->direction = di->spin = 0;
                } else if (di->kind == 0x28) {
                    di->direction = 1;
                    di->spin = 0x14;
                }
            }
        }
    } else if (part->direction == -1 && part->form != 0) {
        part->form--;
    }

    if (part->form != part->form_prev) {
        part_setup_bellow(part);

        if (part->form_prev == 0 || part->form_prev == 2)
            play_sound(0x12);

        place_object_for_draw(part);
    }
}
