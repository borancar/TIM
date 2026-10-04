/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The gun**: its step, setup, flip and drive.
 *
 * The twenty-third module of the original's **code segment 172c**, image
 * 0x1956e..0x19790 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3314..0x3330
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3314..0x3322. Connection points, 7 pairs.
 */
struct point8 g_gun_points_3314[7] = {
    { 0x00, 0x1e }, { 0x07, 0x0a }, { 0x12, 0x01 }, { 0x3f, 0x03 },
    { 0x3f, 0x09 }, { 0x1b, 0x10 }, { 0x0c, 0x1e },
};

/*
 * DGROUP 0x3322..0x3330. Connection points, 7 pairs.
 */
struct point8 g_gun_points_3322[7] = {
    { 0x00, 0x03 }, { 0x2d, 0x01 }, { 0x38, 0x0a }, { 0x3f, 0x1e },
    { 0x33, 0x1e }, { 0x24, 0x10 }, { 0x00, 0x09 },
};

/*
 * 172c:22ae, image 0x1956e - kind 27's step. The gun.
 *
 * Six frames once it is set going, the second playing sound 0x0b, and the
 * third fires: `make_part` builds a kind 0x14, `insert_sorted` puts it on the
 * list at DGROUP 0x5179, and it gets a position and a sideways velocity left
 * or right by the mirror bit - and, mirrored, its own setup at 172c:08a1 runs
 * as well, because the bullet is a different shape that way round.
 *
 * A gun that could not get the bullet from the heap simply does not fire.
 */
void part_step_gun(struct part *part)
{
    struct part *si;

    if (part->direction != 0 && part->form != 6) {
        part->form++;
        place_object_for_draw(part);

        if (part->form == 2)
            play_sound(0x0b);

        if (part->form == 3 && (si = make_part(KIND_BULLET)) != NULL) {
            insert_sorted(si, &g_moving_parts);
            si->traits |= TRAIT_SPAWNED;

            if (part->state & STATE_FLIP_HORIZONTAL) {
                si->state |= STATE_FLIP_HORIZONTAL;
                part_setup_bullet(si);

                si->pos[0].x = part->pos[0].x - 0x20;
                si->pos[1].x = si->pos[2].x = si->pos[0].x + 0x18;
                si->vel_x = (int16_t)0xd000;
            } else {
                si->pos[0].x = part->pos[0].x + 0x24;
                si->pos[1].x = si->pos[2].x = si->pos[0].x - 0x18;
                si->vel_x = 0x3000;
            }

            si->pos[0].y = si->pos[1].y = si->pos[2].y = part->pos[0].y + 3;

            clamp_record_pair(si);

            si->fx = si->pos[0].x;
            si->fx <<= 9;
            si->fy = si->pos[0].y;
            si->fy <<= 9;

            place_object_for_draw(si);
        }
    }
}

/*
 * 172c:23b1, image 0x19671 - a setup.
 *
 * Seven points, with the grab box's width at +0x6a following the same flag:
 * 0x2a with the table at 0x3322, 0x12 with 0x3314. The height at +0x6b is 0x12
 * either way, so one way round the part is square and the other it is not.
 */
void part_setup_gun(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL) {
        part->attach[0].x = 42;
        src = g_gun_points_3322;
    } else {
        part->attach[0].x = 18;
        src = g_gun_points_3314;
    }

    part->attach[0].y = 18;

    for (i = 0, dst = part->points; i < 7; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2412, image 0x196d2 - kind 27's flip: bit 4, its setup, and all four
 * redraws - the draw and the three marks.
 */
void part_flip_gun(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_gun(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2451, image 0x19711 - kind 27's drive hook.
 *
 * Flags of exactly 1 is the counting pass the other drive hooks recognise: the
 * rope's +0x0e goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise only bits 3, 4 and 15 of the flags are kept, and bit 15 is then
 * dropped again for the comparisons - so the drive is read twice, once with
 * the top bit and once without, and the two readings do different jobs. Which
 * of bits 3 and 4 means "the way this part faces" depends on bit 4 of its own
 * +8, and the two halves are mirror images with 8 and 0x10 swapped.
 *
 * Driven **against** the way it faces it refuses, answering 1, which ends the
 * caller's walk. Driven with it while already going it also refuses - it has
 * nothing left to give. Driven with it while stopped, and with the top bit
 * clear, it starts: +0x12 becomes 1 and the answer is 0 so the walk goes on
 * past it.
 */
uint16_t part_drive_gun(struct part *p1, struct part *si, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t p6)
{
    struct rope *di = si->rope[0];
    uint16_t low;                       /* cx */

    if (flags == 1) {
        di->v[0]++;
        return 0;
    }

    flags &= 0x8018;
    low = flags & 0x7fff;

    if (si->state & STATE_FLIP_HORIZONTAL) {
        if (low == 8)
            goto yes;
        if (low == 0x10 && si->direction != 0)
            goto yes;
        if (flags == 0x10 && si->direction == 0)
            si->direction = 1;
    } else {
        if (low == 0x10)
            goto yes;
        if (low == 8 && si->direction != 0) {
yes:
            return 1;
        }
        if (flags == 8 && si->direction == 0)
            si->direction = 1;
    }

    return 0;
}
