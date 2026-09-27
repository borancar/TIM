/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The dynamite plunger**: its hit, setup, step, flip and drive.
 *
 * The thirty-second module of the original's **code segment 172c**, image
 * 0x1a4ff..0x1a72f - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x33ce..0x3422
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * DGROUP 0x33ce..0x33d6. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33CE[4] DGROUP_AT(0x33ce) = {
    { 0x67, 0x00 }, { 0x86, 0x00 }, { 0x7f, 0x2f }, { 0x6f, 0x2f },
};

/*
 * DGROUP 0x33d6..0x33de. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33D6[4] DGROUP_AT(0x33d6) = {
    { 0x67, 0x05 }, { 0x86, 0x05 }, { 0x7f, 0x2f }, { 0x6f, 0x2f },
};

/*
 * DGROUP 0x33de..0x33e6. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33DE[4] DGROUP_AT(0x33de) = {
    { 0x67, 0x0a }, { 0x86, 0x0a }, { 0x7f, 0x2f }, { 0x6f, 0x2f },
};

/*
 * DGROUP 0x33e6..0x33ec. **Which table of points, by form**: a near pointer each.
 */
struct point8 *DYNAMITE_PLUNGER_POINT_TABLE_33E6[3] DGROUP_WAS(0x33e6) = {
    DYNAMITE_PLUNGER_POINTS_33CE, DYNAMITE_PLUNGER_POINTS_33D6,
    DYNAMITE_PLUNGER_POINTS_33DE,
};

/*
 * DGROUP 0x33ec..0x33f4. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33EC[4] DGROUP_AT(0x33ec) = {
    { 0x00, 0x00 }, { 0x1f, 0x00 }, { 0x18, 0x2f }, { 0x08, 0x2f },
};

/*
 * DGROUP 0x33f4..0x33fc. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33F4[4] DGROUP_AT(0x33f4) = {
    { 0x00, 0x05 }, { 0x1f, 0x05 }, { 0x18, 0x2f }, { 0x08, 0x2f },
};

/*
 * DGROUP 0x33fc..0x3404. Connection points, 4 pairs.
 */
struct point8 DYNAMITE_PLUNGER_POINTS_33FC[4] DGROUP_AT(0x33fc) = {
    { 0x00, 0x0a }, { 0x1f, 0x0a }, { 0x18, 0x2f }, { 0x08, 0x2f },
};

/*
 * DGROUP 0x3404..0x340a. **Which table of points, by form**: a near pointer each.
 */
struct point8 *DYNAMITE_PLUNGER_POINT_TABLE_3404[3] DGROUP_WAS(0x3404) = {
    DYNAMITE_PLUNGER_POINTS_33EC, DYNAMITE_PLUNGER_POINTS_33F4,
    DYNAMITE_PLUNGER_POINTS_33FC,
};

/*
 * DGROUP 0x340a..0x3416. Connection points, 3 points.
 */
struct point16 DYNAMITE_PLUNGER_POINTS_340A[3] DGROUP_AT(0x340a) = {
    { 0x0072, 0x0000 }, { 0x0072, 0x0005 }, { 0x0072, 0x000a },
};

/*
 * DGROUP 0x3416..0x3422. Connection points, 3 points.
 */
struct point16 DYNAMITE_PLUNGER_POINTS_3416[3] DGROUP_AT(0x3416) = {
    { 0x000b, 0x0000 }, { 0x000b, 0x0005 }, { 0x000b, 0x000a },
};

/*
 * 172c:323f, image 0x1a4ff - kind 22's hit test, the trigger for
 * `part_step_dynamite_plunger`.
 *
 * A touch on face 0 sets the part going outright. Any other face has to be
 * something landing on it: the arriving object's +0x38 must be positive, which
 * is the downward half of the velocity pair, its +0x88 must be under 0x800
 * once 0x800 is added - so between -0x800 and 0x800, a shallow angle - and it
 * must be above the part, its +0x20 plus +0x42 short of the part's +0x20 by
 * more than 0xc.
 *
 * It answers 1 either way, like every other hit test here.
 */
uint16_t part_hit_dynamite_plunger(struct part *part)
{
    struct part *di = PART_PTR(part->contact_ptr);
    int16_t face = part->contact_edge;  /* [bp-2] */

    if (face == 0)
        di->direction = 1;
    else if (part->vel_y > 0
             && (int16_t)(part->contact_angle + 0x800) < 0x1000
             && part->pos[0].y + part->mirror_size.height < di->pos[0].y + 0x0c)
        di->direction = 1;

    return 1;
}

/*
 * 172c:3294, image 0x1a554 - a setup.
 *
 * Four points and a grab box, all three read out of tables indexed by the form
 * at +0x0c, and bit 4 of +8 picks which set of three tables. The points come
 * through a **pointer array** - the load is a word - and the two box bytes out
 * of four-byte rows, so the same form indexes two different strides.
 */
void part_setup_dynamite_plunger(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        src = DYNAMITE_PLUNGER_POINT_TABLE_3404[part->form];
        part->attach[0].x = (uint8_t)DYNAMITE_PLUNGER_POINTS_3416[part->form].x;
        part->attach[0].y = (uint8_t)DYNAMITE_PLUNGER_POINTS_3416[part->form].y;
    } else {
        src = DYNAMITE_PLUNGER_POINT_TABLE_33E6[part->form];
        part->attach[0].x = (uint8_t)DYNAMITE_PLUNGER_POINTS_340A[part->form].x;
        part->attach[0].y = (uint8_t)DYNAMITE_PLUNGER_POINTS_340A[part->form].y;
    }

    for (i = 0, dst = POINTS(part->points_ptr); i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:332a, image 0x1a5ea - kind 22's step. **It makes a new part.**
 *
 * On the first frame of its three - +0x0c at 1 - it calls `make_part` for a
 * kind-0x29 part, files it on the list at 0x521b, and puts it half a part to
 * the left of itself, or 0x60 to the right of that when bit 4 of the flags
 * says it is mirrored. The new part's 32-bit position at +0x16 and +0x1a is
 * the 16-bit one shifted left nine, which is the fixed point the physics uses.
 *
 * `make_part` answering zero is a full heap and is simply skipped; the part
 * still steps its own form.
 *
 * The form then walks 1, 2 and stops, rebuilt each time through the setup at
 * 172c:3294, called directly.
 */
void part_step_dynamite_plunger(struct part *part)
{
    struct part *si;

    if (part->direction != 0) {
        if (part->form == 1) {
            play_sound(8);

            if ((si = make_part(KIND_BLAST)) == PART_NONE)
                goto done;

            insert_sorted(si, &DG521B.placed_parts);
            si->flags_06 |= 0x10;
            si->pos[0].x = part->pos[0].x - 0x10;
            si->pos[0].y = part->pos[0].y;

            if (part->flags_08 & 0x10)
                si->pos[0].x += 0x60;

            si->fx = si->pos[0].x;
            si->fx <<= 9;
            si->fy = si->pos[0].y;
            si->fy <<= 9;

            place_object_for_draw(si);
        }
done:
        if (part->form != 2) {
            part->form++;
            part_setup_dynamite_plunger(part);
            place_object_for_draw(part);
        }
    }
}

/*
 * 172c:33e5, image 0x1a6a5 - kind 22's flip: bit 4, its setup, three marks and
 * no draw.
 */
void part_flip_dynamite_plunger(struct part *part)
{
    part->flags_08 ^= 0x10;
    part_setup_dynamite_plunger(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:341d, image 0x1a6dd - kind 22's drive, the same family as kind 31's at
 * 172c:2e4b: the mode masked to 0x8006 and then to 0x7fff, which leaves 2, 4
 * or 6 and drops the top bit, and the second test asking about the *0x8006*
 * value so a 4 with the top bit on takes neither arm.
 *
 * Mode 2 always answers yes and mode 4 answers yes once the form has reached
 * 2. Failing that, a plain 4 sets +0x12 going if it is not going already, and
 * answers 0 - so this is the drive that starts a kind 22 rather than reporting
 * on it.
 */
uint16_t part_drive_341d(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *di = BELT_PTR(p2->belt_ptr[0]);
    uint16_t low;                       /* cx */

    if (p4 == 1) {
        di->v[0]++;
        return 0;
    }

    p4 &= 0x8006;
    low = p4 & 0x7fff;

    if (low == 2)
        goto yes;
    if (low == 4 && p2->form == 2) {
yes:
        return 1;
    }

    if (p4 == 4 && p2->direction == 0)
        p2->direction = 1;

    return 0;
}
